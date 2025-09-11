#ifndef SHAM_H
#define SHAM_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define MAX_PAYLOAD 1024
#define MAX_WINDOW 10           // number of packets in flight allowed
#define TIMEOUT_MS 500         // retransmission timeout in ms
#define DEFAULT_BUFFER 8192    // receiver buffer in bytes

typedef struct sham_header {
    uint32_t seq_num;       // byte-based sequence number
    uint32_t ack_num;       // cumulative ack (next expected byte)
    uint16_t flags;         // SYN, ACK, FIN
    uint16_t window_size;   // receiver advertised window (bytes)
} sham_header;

#define SYN 0x1
#define ACK 0x2
#define FIN 0x4

typedef struct sham_packet {
    sham_header header;
    char data[MAX_PAYLOAD];
    size_t data_len; // valid only for data packets
} sham_packet;

// Logging helper (use RUDP_LOG=1 to enable)
static inline void log_event(const char *role, const char *message) {
    char *env = getenv("RUDP_LOG");
    if (!env || strcmp(env, "1") != 0) return;

    char filename[32];
    snprintf(filename, sizeof(filename), "%s_log.txt", role);

    FILE *log_file = fopen(filename, "a");
    if (!log_file) return;

    char time_buffer[30];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t curtime = tv.tv_sec;
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", localtime(&curtime));

    fprintf(log_file, "[%s.%06ld] [LOG] %s\n", time_buffer, tv.tv_usec, message);
    fclose(log_file);
}

#endif
