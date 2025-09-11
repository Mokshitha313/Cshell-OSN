#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include "intrinsics.h"
#include "execute.h"
#include "prompt.h"
#include "log.h"
#include "hop.h"
#include "reveal.h"
#include "parser.h"
#include "jobs.h"
#include "common.h"

#define MAX_INPUT 255
#define MAX_TOKENS 128

char input_buf[MAX_INPUT];

// Signal handlers from intrinsics.c
extern void sigint_handler(int sig);
extern void sigtstp_handler(int sig);

// ---------------- SIGCHLD handler ----------------
void sigchld_handler(int sig) {
    (void)sig;  // suppress unused warning
    int saved_errno = errno;

    // Reap all finished child processes
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        jobs_check();  // mark completed jobs and print messages
    }

    errno = saved_errno;
}

// ---------------- Parse and execute commands ----------------
void parse_and_execute(char *input, const char *original_input) {
    char *cmd_ptr = input;

    while (*cmd_ptr) {
        char *sep = cmd_ptr;
        int run_in_background = 0;

        while (*sep && *sep != ';' && *sep != '&') sep++;
        if (*sep == '&') run_in_background = 1;

        char tmp = *sep;
        *sep = '\0';

        while (*cmd_ptr && isspace((unsigned char)*cmd_ptr)) cmd_ptr++;
        if (*cmd_ptr == '\0') {
            if (tmp) *sep = tmp;
            cmd_ptr = (tmp) ? sep + 1 : sep;
            continue;
        }

        // Handle pipelines
        if (strchr(cmd_ptr, '|') != NULL) {
            execute_pipeline(cmd_ptr);
        } else {
            // Tokenize arguments
            char *args[MAX_TOKENS];
            int argc = 0;
            char *p = cmd_ptr;

            while (*p && argc < MAX_TOKENS - 1) {
                while (*p && isspace((unsigned char)*p)) p++;
                if (!*p) break;

                if (*p == '>' && *(p+1) == '>') {
                    args[argc++] = strdup(">>");
                    p += 2;
                } else if (*p == '>' || *p == '<') {
                    char op[2] = {*p, '\0'};
                    args[argc++] = strdup(op);
                    p++;
                } else {
                    char *start = p;
                    while (*p && !isspace((unsigned char)*p) && *p != '>' && *p != '<') p++;
                    size_t len = p - start;
                    char *tok = malloc(len + 1);
                    memcpy(tok, start, len);
                    tok[len] = '\0';
                    args[argc++] = tok;
                }
            }
            args[argc] = NULL;

            if (argc > 0) {
                // ---------------- Built-in commands ----------------
                if (strcmp(args[0], "hop") == 0)
                    hop_command(argc, args);
                else if (strcmp(args[0], "reveal") == 0)
                    execute_command(argc, args);
                else if (strcmp(args[0], "log") == 0)
                    log_command(argc, args, original_input);
                else if (strcmp(args[0], "activities") == 0)
                    activities();
                else if (strcmp(args[0], "ping") == 0) {
                    if (argc < 3)
                        printf("Usage: ping <pid> <signal_number>\n");
                    else
                        ping(atoi(args[1]), atoi(args[2]));
                }
                else if (strcmp(args[0], "fg") == 0) {
                    if (argc == 1) fg(-1);
                    else fg(atoi(args[1]));
                }
                else if (strcmp(args[0], "bg") == 0) {
                    if (argc == 1)
                        printf("Usage: bg <job_number>\n");
                    else
                        bg(atoi(args[1]));
                }
                // ---------------- External commands ----------------
                else {
                    pid_t pid = fork();
                    if (pid == 0) {
                        if (run_in_background) setpgid(0, 0);
                        execute_command(argc, args);
                        exit(0);
                    } else if (pid > 0) {
                        if (run_in_background) {
                            add_job(pid, cmd_ptr, RUNNING);
                            printf("[%d] %d\n", job_count, pid);
                        } else {
                            strncpy(current_fg_command, cmd_ptr, MAX_COMMAND_LEN-1);
                            current_fg_command[MAX_COMMAND_LEN-1] = '\0';
                            foreground_pid = pid;
                            int status;
                            waitpid(pid, &status, WUNTRACED);
                            foreground_pid = -1;

                            if (WIFSTOPPED(status))
                                add_job(pid, current_fg_command, STOPPED);
                        }
                    } else {
                        printf("Command not found!\n");
                        exit(1);
                    }
                }
            }

            for (int i = 0; i < argc; i++) free(args[i]);
        }

        if (tmp) *sep = tmp;
        cmd_ptr = (tmp) ? sep + 1 : sep;
    }
}

// ---------------- Main ----------------
int main() {
    init_prompt();
    log_init();

    // Install signal handlers
    signal(SIGCHLD, sigchld_handler);  // handle background job completion
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);

    while (1) {
        jobs_check();
        print_prompt();

        if (!fgets(input_buf, sizeof(input_buf), stdin)) {
            if (feof(stdin)) handle_eof();
            continue;
        }

        input_buf[strcspn(input_buf, "\n")] = '\0';
        if (strlen(input_buf) == 0) continue;

        char original_input[MAX_INPUT];
        strncpy(original_input, input_buf, MAX_INPUT-1);
        original_input[MAX_INPUT-1] = '\0';

        if (!parse_command(input_buf)) continue;

        log_store(original_input); // keep log only once

        parse_and_execute(input_buf, original_input);
    }

    return 0;
}
