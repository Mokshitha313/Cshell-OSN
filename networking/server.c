#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#define OPENSSL_API_COMPAT 0x10100000L
#include <openssl/md5.h>
#include <time.h>
#include "sham.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

void calculate_md5(const char *filename) {
    unsigned char c[MD5_DIGEST_LENGTH];
    int i;
    FILE *inFile = fopen(filename, "rb");
    if (!inFile) { perror("fopen"); return; }
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];
    MD5_Init(&mdContext);
    while ((bytes = fread(data, 1, 1024, inFile)) != 0)
        MD5_Update(&mdContext, data, bytes);
    MD5_Final(c, &mdContext);
    printf("MD5: ");
    for (i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", c[i]);
    printf("\n");
    fclose(inFile);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <port> [--chat] [loss_rate]\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int chat_mode = 0;
    double loss_rate = 0.0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--chat") == 0) chat_mode = 1;
        else loss_rate = atof(argv[i]);
    }

    /* seed randomness for simulated loss */
    srand((unsigned)time(NULL) ^ (unsigned)port);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); exit(1);
    }

    printf("Server listening on port %d...\n", port);

    sham_packet packet;
    memset(&packet, 0, sizeof(packet));

    // --- Handshake ---
    ssize_t n = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &addr_len);
    if (n <= 0) {
        perror("recvfrom"); close(sockfd); exit(1);
    }

    if (packet.header.flags & SYN) {
        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "RCV SYN SEQ=%u", packet.header.seq_num);
        log_event("server", log_msg);

        // send SYN-ACK
        sham_packet synack;
        memset(&synack, 0, sizeof(synack));
        synack.header.seq_num = 5000; // server initial seq
        synack.header.ack_num = packet.header.seq_num + 1;
        synack.header.flags = SYN | ACK;
        sendto(sockfd, &synack, sizeof(synack), 0, (struct sockaddr *)&client_addr, addr_len);
        snprintf(log_msg, sizeof(log_msg), "SND SYN-ACK SEQ=%u ACK=%u", synack.header.seq_num, synack.header.ack_num);
        log_event("server", log_msg);

        // receive ACK
        n = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (n > 0 && (packet.header.flags & ACK)) {
            log_event("server", "RCV ACK FOR SYN");
        }
    }

    if (!chat_mode) {
        char filename[256] = "received_file"; // default fallback
        FILE *fp = NULL;

        uint32_t expected_seq = 1;
        int filename_received = 0;

        while (1) {
            ssize_t nbytes = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &addr_len);
            if (nbytes <= 0) continue;

            // Simulate packet loss for data packets (don't drop control packets)
            if (!(packet.header.flags & (SYN|ACK|FIN)) && ((double)rand()/RAND_MAX) < loss_rate) {
                char log_msg[128];
                snprintf(log_msg, sizeof(log_msg), "DROP DATA SEQ=%u", packet.header.seq_num);
                log_event("server", log_msg);
                continue;
            }

            // Handle FIN
            if (packet.header.flags & FIN) {
                log_event("server", "RCV FIN SEQ");
                sham_packet ack_fin;
                memset(&ack_fin, 0, sizeof(ack_fin));
                ack_fin.header.flags = ACK;
                sendto(sockfd, &ack_fin, sizeof(ack_fin), 0, (struct sockaddr *)&client_addr, addr_len);
                break;
            }

            // First non-control packet with seq_num == 0 is treated as filename
            if (!filename_received && packet.header.seq_num == 0 && packet.data_len > 0) {
                size_t fn_len = packet.data_len < sizeof(filename)-1 ? packet.data_len : sizeof(filename)-1;
                memcpy(filename, packet.data, fn_len);
                filename[fn_len] = '\0';
                filename_received = 1;

                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "RCV FILENAME %.240s", filename);
                log_event("server", log_msg);

                fp = fopen(filename, "wb");
                if (!fp) { perror("fopen"); exit(1); }

                // send ACK for filename (ack_num stays as expected_seq which is 1)
                sham_packet ack_pkt;
                memset(&ack_pkt, 0, sizeof(ack_pkt));
                ack_pkt.header.flags = ACK;
                ack_pkt.header.ack_num = expected_seq;
                ack_pkt.header.window_size = DEFAULT_BUFFER;
                sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)&client_addr, addr_len);
                snprintf(log_msg, sizeof(log_msg), "SND ACK=%u WIN=%u", ack_pkt.header.ack_num, ack_pkt.header.window_size);
                log_event("server", log_msg);

                continue; // wait for real file data
            }

            // If file pointer not opened yet, open default file
            if (!fp) {
                fp = fopen(filename, "wb");
                if (!fp) { perror("fopen"); exit(1); }
            }

            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "RCV DATA SEQ=%u LEN=%zu", packet.header.seq_num, packet.data_len);
            log_event("server", log_msg);

            if (packet.header.seq_num == expected_seq) {
                fwrite(packet.data, 1, packet.data_len, fp);
                expected_seq += packet.data_len;
            } else {
                // out-of-order: buffer logic could be added here; for now we just ignore but ACK expected_seq
            }

            // send cumulative ACK
            sham_packet ack_pkt;
            memset(&ack_pkt, 0, sizeof(ack_pkt));
            ack_pkt.header.flags = ACK;
            ack_pkt.header.ack_num = expected_seq;
            ack_pkt.header.window_size = DEFAULT_BUFFER;
            sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)&client_addr, addr_len);

            snprintf(log_msg, sizeof(log_msg), "SND ACK=%u WIN=%u", ack_pkt.header.ack_num, ack_pkt.header.window_size);
            log_event("server", log_msg);
        }

        if (fp) fclose(fp);
        calculate_md5(filename);
    } 
    else {
        printf("Chat started. Type /quit to exit.\n");
        fd_set readfds;
        while (1) {
            FD_ZERO(&readfds);
            FD_SET(0, &readfds);
            FD_SET(sockfd, &readfds);
            int maxfd = sockfd;
            select(maxfd + 1, &readfds, NULL, NULL, NULL);

            if (FD_ISSET(0, &readfds)) {
                char buf[1024];
                if (!fgets(buf, sizeof(buf), stdin)) continue;
                buf[strcspn(buf, "\n")] = 0;

                sham_packet msg;
                memset(&msg, 0, sizeof(msg));
                msg.data_len = strlen(buf);
                strncpy(msg.data, buf, msg.data_len);

                if (strcmp(buf, "/quit") == 0) {
                    msg.header.flags = FIN; // inform client to terminate
                    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&client_addr, addr_len);
                    printf("Chat session terminated.\n");
                    break;
                }

                sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&client_addr, addr_len);
            }

            if (FD_ISSET(sockfd, &readfds)) {
                sham_packet msg;
                ssize_t n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&client_addr, &addr_len);
                if (n > 0) {
                    if (msg.header.flags & FIN) {
                        printf("Client has terminated the chat.\n");
                        break;
                    }
                    if (msg.data_len > 0)
                        printf("Client: %s\n", msg.data);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}