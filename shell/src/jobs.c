#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

job jobs[MAX_JOBS];
int job_count = 0;

// ---------------- Add a job ----------------
void add_job(pid_t pid, const char *command, job_state state) {
    if (job_count >= MAX_JOBS) return;
    jobs[job_count].pid = pid;
    jobs[job_count].state = state;
    strncpy(jobs[job_count].command, command, MAX_COMMAND_LEN-1);
    jobs[job_count].command[MAX_COMMAND_LEN-1] = '\0';
    jobs[job_count].job_number = job_count + 1;
    job_count++;
}

// ---------------- Remove a job ----------------
void remove_job(int index) {
    if (index < 0 || index >= job_count) return;
    for (int i = index; i < job_count - 1; i++) {
        jobs[i] = jobs[i + 1];
        jobs[i].job_number = i + 1;
    }
    job_count--;
}

// ---------------- Check finished jobs ----------------
void jobs_check(void) {
    int status;
    for (int i = 0; i < job_count; i++) {
        pid_t ret = waitpid(jobs[i].pid, &status, WNOHANG | WUNTRACED);
        if (ret > 0) {
            if (WIFEXITED(status)) {
                printf("[%d] %d Done\n", i + 1, jobs[i].pid);
                fflush(stdout);
                remove_job(i);
                i--;
            } else if (WIFSIGNALED(status)) {
                printf("[%d] %d Terminated\n", i + 1, jobs[i].pid);
                fflush(stdout);
                remove_job(i);
                i--;
            } else if (WIFSTOPPED(status)) {
                jobs[i].state = STOPPED;
            } else if (WIFCONTINUED(status)) {
                jobs[i].state = RUNNING;
            }
        }
    }
}
