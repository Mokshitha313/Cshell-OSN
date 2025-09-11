#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include "intrinsics.h"
#include "jobs.h"

// Foreground process info
pid_t foreground_pid = -1;
char current_fg_command[MAX_COMMAND_LEN] = "";

// ---------------- Ctrl-C handler ----------------
void sigint_handler(int sig) {
    (void)sig;
    if (foreground_pid > 0) {
        kill(-foreground_pid, SIGINT);
    }
}

// ---------------- Ctrl-Z handler ----------------
void sigtstp_handler(int sig) {
    (void)sig;
    if (foreground_pid > 0) {
        kill(-foreground_pid, SIGTSTP);
        add_job(foreground_pid, current_fg_command, STOPPED);
        printf("\n[%d] Stopped %s\n", job_count, current_fg_command);
        foreground_pid = -1;
    }
}

// ---------------- Ctrl-D handler ----------------
void handle_eof(void) {
    printf("logout\n");
    for (int i = 0; i < job_count; i++) {
        kill(jobs[i].pid, SIGKILL);
    }
    exit(0);
}

// ---------------- activities ----------------
void activities(void) {
    jobs_check();

    // Sort lexicographically by command name
    for (int i = 0; i < job_count - 1; i++) {
        for (int j = i + 1; j < job_count; j++) {
            if (strcmp(jobs[i].command, jobs[j].command) > 0) {
                job temp = jobs[i];
                jobs[i] = jobs[j];
                jobs[j] = temp;
            }
        }
    }

    for (int i = 0; i < job_count; i++) {
        printf("[%d] : %s - %s\n", jobs[i].pid, jobs[i].command,
               jobs[i].state == RUNNING ? "Running" : "Stopped");
    }
}

// ---------------- ping ----------------
void ping(int pid, int sig) {
    int actual_signal = sig % 32;
    if (kill(pid, 0) == -1) {
        printf("No such process found\n");
        return;
    }

    if (kill(pid, actual_signal) == 0) {
        printf("Sent signal %d to process with pid %d\n", sig, pid);
    } else {
        perror("Error sending signal");
    }
}

// ---------------- fg command ----------------
void fg(int job_number) {
    int idx = -1;
    if (job_number == -1) {
        if (job_count > 0) idx = job_count - 1;
    } else {
        for (int i = 0; i < job_count; i++)
            if (jobs[i].job_number == job_number) idx = i;
    }

    if (idx == -1) {
        printf("No such job\n");
        return;
    }

    printf("%s\n", jobs[idx].command);

    foreground_pid = jobs[idx].pid;
    strncpy(current_fg_command, jobs[idx].command, MAX_COMMAND_LEN-1);

    if (jobs[idx].state == STOPPED)
        kill(jobs[idx].pid, SIGCONT);

    int status;
    waitpid(jobs[idx].pid, &status, WUNTRACED);
    foreground_pid = -1;

    if (WIFSTOPPED(status))
        jobs[idx].state = STOPPED;
    else
        remove_job(idx);
}

// ---------------- bg command ----------------
void bg(int job_number) {
    int idx = -1;
    if (job_number == -1) {
        if (job_count > 0) idx = job_count - 1;
    } else {
        for (int i = 0; i < job_count; i++)
            if (jobs[i].job_number == job_number) idx = i;
    }

    if (idx == -1) {
        printf("No such job\n");
        return;
    }

    if (jobs[idx].state == RUNNING) {
        printf("Job already running\n");
        return;
    }

    kill(jobs[idx].pid, SIGCONT);
    jobs[idx].state = RUNNING;
    printf("[%d] %s &\n", jobs[idx].job_number, jobs[idx].command);
}
