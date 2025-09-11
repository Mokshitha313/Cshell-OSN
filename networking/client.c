#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <openssl/md5.h>
#include "sham.h"

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: %s <server_ip> <server_port> <input_file> <output_file> [--chat] [loss_rate]\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char *input_file = argv[3];
    char *output_file = argv[4];
    int chat_mode = 0;
    double loss_rate = 0.0;

    int argi = 5;
    if (argc > argi && strcmp(argv[argi], "--chat") == 0) {
        chat_mode = 1;
        argi++;
    }
    if (argc > argi) loss_rate = atof(argv[argi]);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    sham_packet packet;
    memset(&packet, 0, sizeof(packet));

    // --- Handshake ---
    packet.header.seq_num = 100;
    packet.header.flags = SYN;
    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, addr_len);
    log_event("client", "SND SYN SEQ=100");

    recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, &addr_len);
    if ((packet.header.flags & SYN) && (packet.header.flags & ACK)) {
        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "RCV SYN SEQ=%u ACK=%u", packet.header.seq_num, packet.header.ack_num);
        log_event("client", log_msg);

        // send ACK
        sham_packet ack_pkt;
        memset(&ack_pkt, 0, sizeof(ack_pkt));
        ack_pkt.header.flags = ACK;
        ack_pkt.header.ack_num = packet.header.seq_num + 1;
        sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)&server_addr, addr_len);
        log_event("client", "SND ACK SEQ=101 ACK=5001");
    }

    if (!chat_mode) {
        // --- Send output filename first (seq_num = 0) ---
        sham_packet fname_pkt;
        memset(&fname_pkt, 0, sizeof(fname_pkt));
        fname_pkt.header.seq_num = 0;
        fname_pkt.data_len = strlen(output_file);
        strncpy(fname_pkt.data, output_file, sizeof(fname_pkt.data) - 1);
        sendto(sockfd, &fname_pkt, sizeof(fname_pkt), 0, (struct sockaddr *)&server_addr, addr_len);

        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "SND FILENAME='%s'", output_file);
        log_event("client", log_msg);

        // wait for ACK of filename
        sham_packet fname_ack;
        recvfrom(sockfd, &fname_ack, sizeof(fname_ack), 0, (struct sockaddr *)&server_addr, &addr_len);

        // --- Send file data ---
        FILE *fp = fopen(input_file, "rb");
        if (!fp) { perror("fopen"); exit(1); }
        uint32_t seq = 1;
        size_t bytes;
        char buf[MAX_PAYLOAD];
        while ((bytes = fread(buf, 1, MAX_PAYLOAD, fp)) > 0) {
            sham_packet data_pkt;
            memset(&data_pkt, 0, sizeof(data_pkt));
            data_pkt.header.seq_num = seq;
            data_pkt.data_len = bytes;
            memcpy(data_pkt.data, buf, bytes);

            sendto(sockfd, &data_pkt, sizeof(data_pkt), 0, (struct sockaddr *)&server_addr, addr_len);

            snprintf(log_msg, sizeof(log_msg), "SND DATA SEQ=%u LEN=%zu", seq, bytes);
            log_event("client", log_msg);

            seq += bytes;

            // receive ACK
            sham_packet ack;
            recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&server_addr, &addr_len);
            snprintf(log_msg, sizeof(log_msg), "RCV ACK=%u", ack.header.ack_num);
            log_event("client", log_msg);
        }

        // --- Send FIN ---
        sham_packet fin_pkt;
        memset(&fin_pkt, 0, sizeof(fin_pkt));
        fin_pkt.header.flags = FIN;
        sendto(sockfd, &fin_pkt, sizeof(fin_pkt), 0, (struct sockaddr *)&server_addr, addr_len);
        log_event("client", "SND FIN");

        // receive ACK for FIN
        sham_packet ack;
        recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&server_addr, &addr_len);
        log_event("client", "RCV ACK FOR FIN");

        fclose(fp);
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
                    msg.header.flags = FIN; // inform server to terminate
                    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&server_addr, addr_len);
                    printf("Chat session terminated.\n");
                    break;
                }

                sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&server_addr, addr_len);
            }

            if (FD_ISSET(sockfd, &readfds)) {
                sham_packet msg;
                ssize_t n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&server_addr, &addr_len);
                if (n > 0) {
                    if (msg.header.flags & FIN) {
                        printf("Server has terminated the chat.\n");
                        break;
                    }
                    if (msg.data_len > 0)
                        printf("Server: %s\n", msg.data);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
