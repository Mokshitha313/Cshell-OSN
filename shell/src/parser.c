#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

int parse_command(const char *input) {
    // Reject empty input
    if (input == NULL || strlen(input) == 0) return 0;

    // Simple invalid check: no "||" allowed, no command starting with ; or |
    if (strstr(input, "||") != NULL || strstr(input, "|;") != NULL) {
        printf("Invalid Syntax!\n");
        return 0;
    }

    // If input only has special symbols, reject
    int only_symbols = 1;
    for (size_t i = 0; i < strlen(input); i++) {
        if (!strchr("|&><;", input[i])) {
            only_symbols = 0;
            break;
        }
    }
    if (only_symbols) {
        printf("Invalid Syntax!\n");
        return 0;
    }

    return 1; // valid
}


