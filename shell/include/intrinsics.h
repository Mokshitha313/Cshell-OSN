#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define MAX_COMMAND_LEN 256

// Foreground process tracking
extern pid_t foreground_pid;
extern char current_fg_command[MAX_COMMAND_LEN];

// Signal handlers
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void handle_eof(void);

// Intrinsic commands
void activities(void);
void ping(int pid, int sig);
void fg(int job_number);
void bg(int job_number);

#endif
