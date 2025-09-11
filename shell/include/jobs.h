#ifndef JOBS_H
#define JOBS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_JOBS 128
#define MAX_COMMAND_LEN 256

typedef enum { RUNNING, STOPPED } job_state;

typedef struct {
    int job_number;
    pid_t pid;
    char command[MAX_COMMAND_LEN];
    job_state state;
} job;

// Jobs array and count
extern job jobs[MAX_JOBS];
extern int job_count;

// Job functions
void add_job(pid_t pid, const char *command, job_state state);
void remove_job(int index);
void jobs_check(void); // remove finished jobs and update states

#endif
