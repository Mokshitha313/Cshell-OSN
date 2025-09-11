#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include "execute.h"
#include "reveal.h"
#include "log.h"

#define MAX_TOKENS 128

// ---------------- Helper ----------------

static int split_pipeline(char *input, char **commands) {
    int count = 0;
    char *p = strtok(input, "|");
    while (p != NULL && count < MAX_TOKENS) {
        while (*p == ' ' || *p == '\t') p++;
        commands[count++] = p;
        p = strtok(NULL, "|");
    }
    return count;
}

static void parse_redirection(char **args, int *argc, char **input_file, char **output_file, int *append) {
    *input_file = NULL;
    *output_file = NULL;
    *append = 0;

    char *last_input = NULL;
    char *last_output = NULL;
    int last_append = 0;
    int input_error = 0;

    for (int i = 0; i < *argc; i++) {
        // Input redirection
        if (strcmp(args[i], "<") == 0 && i + 1 < *argc) {
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "No such file or directory\n");
                input_error = 1;
            } else {
                close(fd);
                last_input = args[i + 1];
            }

            for (int j = i; j + 2 <= *argc; j++) args[j] = args[j + 2];
            *argc -= 2;
            i--;
        }

        // Output redirection
        else if ((strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0) && i + 1 < *argc) {
            // Try to create the file immediately
            int temp_fd;
            if (strcmp(args[i], ">>") == 0)
                temp_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            else
                temp_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (temp_fd < 0) {
                fprintf(stderr, "Unable to create file for writing\n");
                *argc = 0;
                return;
            }
            close(temp_fd);

            last_output = args[i + 1];
            last_append = (strcmp(args[i], ">>") == 0);

            for (int j = i; j + 2 <= *argc; j++) args[j] = args[j + 2];
            *argc -= 2;
            i--;
        }
    }

    if (input_error && last_input == NULL) {
        *argc = 0;
        return;
    }

    *input_file = last_input;
    *output_file = last_output;
    *append = last_append;
}



static int is_builtin(char *cmd) {
    return (strcmp(cmd, "reveal") == 0) || (strcmp(cmd, "log") == 0);
}

static void execute_builtin(int argc, char **argv) {
    int stdout_backup = dup(STDOUT_FILENO);
    int stdin_backup = dup(STDIN_FILENO);
    char *input_file = NULL, *output_file = NULL;
    int append = 0;

    parse_redirection(argv, &argc, &input_file, &output_file, &append);
    if (argc == 0) goto restore;

    if (input_file) {
        int fd = open(input_file, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "No such file or directory\n");
            goto restore;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (output_file) {
        int fd;
        if (append)
            fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        else
            fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd < 0) {
            fprintf(stderr, "Unable to create file for writing\n");
            goto restore;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    if (strcmp(argv[0], "reveal") == 0) {
        reveal_command(argc, argv);
    } else if (strcmp(argv[0], "log") == 0) {
        log_command(argc, argv, NULL);
    } else {
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    }

restore:
    dup2(stdout_backup, STDOUT_FILENO);
    dup2(stdin_backup, STDIN_FILENO);
    close(stdout_backup);
    close(stdin_backup);
}

void execute_command(int argc, char **argv) {
    if (argc == 0) return;

    if (is_builtin(argv[0])) {
        execute_builtin(argc, argv);
        return;
    }

    char *input_file = NULL, *output_file = NULL;
    int append = 0;
    parse_redirection(argv, &argc, &input_file, &output_file, &append);
    if (argc == 0) return;

    pid_t pid = fork();
    if (pid == 0) {
        if (input_file) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "No such file or directory\n");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        if (output_file) {
            int fd;
            if (append)
                fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            else
                fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (fd < 0) {
                fprintf(stderr, "Unable to create file for writing\n");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(argv[0], argv);
        fprintf(stderr, "Command not found!\n");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        fprintf(stderr, "Command not found!\n");
    }
}

void execute_pipeline(char *input) {
    char *commands[MAX_TOKENS];
    int n = split_pipeline(input, commands);
    int prev_fd = -1;
    pid_t pids[MAX_TOKENS];

    for (int i = 0; i < n; i++) {
        int pipefd[2];
        if (i < n - 1 && pipe(pipefd) == -1) {
            perror("pipe");
            return;
        }

        pid_t pid = fork();
        if (pid == 0) {
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            if (i < n - 1) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }

            char *args[MAX_TOKENS];
            int argc = 0;
            char *tok = strtok(commands[i], " \t\n");
            while (tok != NULL && argc < MAX_TOKENS - 1) {
                args[argc++] = tok;
                tok = strtok(NULL, " \t\n");
            }
            args[argc] = NULL;

            if (argc == 0) exit(0);

            if (is_builtin(args[0])) {
                execute_builtin(argc, args);
                exit(0);
            }

            char *input_file = NULL, *output_file = NULL;
            int append = 0;
            parse_redirection(args, &argc, &input_file, &output_file, &append);

            if (input_file && i == 0) {
                int fd = open(input_file, O_RDONLY);
                if (fd < 0) {
                    fprintf(stderr, "No such file or directory\n");
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            if (output_file && i == n - 1) {
                int fd;
                if (append)
                    fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                else
                    fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                if (fd < 0) {
                    fprintf(stderr, "Unable to create file for writing\n");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            execvp(args[0], args);
            fprintf(stderr, "Command not found!\n");
            exit(1);
        } else if (pid > 0) {
            pids[i] = pid;
            if (prev_fd != -1) close(prev_fd);
            if (i < n - 1) {
                close(pipefd[1]);
                prev_fd = pipefd[0];
            }
        } else {
            perror("fork");
            return;
        }
    }

    if (prev_fd != -1) close(prev_fd);

    for (int i = 0; i < n; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
}