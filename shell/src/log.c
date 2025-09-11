#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "log.h"
#include "execute.h"

#define LOG_FILE ".myshell_history"
#define MAX_LOG 15
#define MAX_CMD 1024

static char history[MAX_LOG][MAX_CMD];           // All commands typed
static char history_indexed[MAX_LOG][MAX_CMD];   // Commands used for execute indexing
static int count = 0;         // total commands typed
static int count_indexed = 0; // commands counted for execute

extern int parse_command(const char *input);

void log_init() {
    FILE *fp = fopen(LOG_FILE, "r");
    if (!fp) return;

    count = 0;
    count_indexed = 0;
    char line[MAX_CMD];
    char prev[MAX_CMD] = "";

    while (fgets(line, MAX_CMD, fp)) {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;

        // skip duplicates
        if (strcmp(line, prev) == 0) continue;
        strncpy(prev, line, MAX_CMD);

        // skip log execute lines completely
        if (strncmp(line, "log execute", 11) == 0) continue;

        strncpy(history[count % MAX_LOG], line, MAX_CMD);
        history[count % MAX_LOG][MAX_CMD - 1] = '\0';
        count++;

        // also add to indexed history
        strncpy(history_indexed[count_indexed % MAX_LOG], line, MAX_CMD);
        history_indexed[count_indexed % MAX_LOG][MAX_CMD - 1] = '\0';
        count_indexed++;
    }
    fclose(fp);
}

static void save_history() {
    FILE *fp = fopen(LOG_FILE, "w");
    if (!fp) return;

    int start = (count > MAX_LOG) ? count - MAX_LOG : 0;
    for (int i = start; i < count; i++) {
        fprintf(fp, "%s\n", history[i % MAX_LOG]);
    }

    fclose(fp);
}

void log_command(int argc, char **argv, const char *raw_input) {
    if (argc == 1) {
        // Print full history
        int start = (count > MAX_LOG) ? count - MAX_LOG : 0;
        for (int i = start; i < count; i++) {
            printf("%s\n", history[i % MAX_LOG]);
        }
        return;
    }

    if (strcmp(argv[1], "purge") == 0) {
        count = 0;
        count_indexed = 0;
        save_history();
        return;
    }

    if (strcmp(argv[1], "execute") == 0 && argc == 3) {
        int index = atoi(argv[2]);
        int start = (count_indexed > MAX_LOG) ? count_indexed - MAX_LOG : 0;
        int size = count_indexed - start;

        if (index < 1 || index > size) {
            printf("Invalid index!\n");
            return;
        }

        char *cmd = history_indexed[(start + size - index) % MAX_LOG];

        // Execute the command directly as a string (handles pipelines)
        execute_pipeline(cmd);
        return;
    }

    printf("log: Invalid Syntax!\n");
}

void log_store(const char *raw_input) {
    static int first_hop_logged = 0;

    if (strcmp(raw_input, "log") == 0) return;             // skip plain log
    if (strncmp(raw_input, "log execute", 11) == 0) return; // skip log execute
    if (!first_hop_logged && strcmp(raw_input, "hop") == 0) {
        first_hop_logged = 1;
        return;
    }

    // skip duplicates
    if (count > 0 && strcmp(history[(count - 1) % MAX_LOG], raw_input) == 0) {
        return;
    }

    // Store in full history
    strncpy(history[count % MAX_LOG], raw_input, MAX_CMD);
    history[count % MAX_LOG][MAX_CMD - 1] = '\0';
    count++;

    // Store in indexed history
    strncpy(history_indexed[count_indexed % MAX_LOG], raw_input, MAX_CMD);
    history_indexed[count_indexed % MAX_LOG][MAX_CMD - 1] = '\0';
    count_indexed++;

    save_history();
}