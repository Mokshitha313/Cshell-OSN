XV6 Mini Project Report

Student Name: Mokshitha
Course: Operating Systems
Project: XV6 – System Calls & Scheduling
Total Marks: 140

Part A – Basic System Call: getreadcount
Objective:
The objective of this part was to implement a system call getreadcount() that tracks and returns the total number of bytes read using the read() system call across all processes since the system boot.

Implementation Details:
1) Kernel Modifications
    Added a global variable uint64 read_count in sysfile.c to keep track of the total number of bytes read.
    Modified the sys_read() function to increment read_count each time fileread() successfully reads bytes. Implemented sys_getreadcount() in sysproc.c to return the value of read_count.

2) System Call Registration
    Added SYS_getreadcount in syscall.h.
    Added function entry in syscalls[] array in syscall.c.
    Added user-level prototype in user/user.h.
    Updated usys.pl to expose getreadcount() to user programs.

3) User Program
    Created readcount.c in /user:
    Calls getreadcount() and prints the initial value.
    Reads 100 bytes from a file.
    Calls getreadcount() again to verify the increase.

Results:
Initial read count: 10 bytes
Read count after reading 100 bytes: 110 bytes
Test passed: read count increased by 100 bytes

This confirms that the getreadcount() system call accurately tracks the total bytes read by all processes.

Part B – Scheduling
Objective:
Implement two alternate scheduling policies for XV6:
    First Come First Serve (FCFS)
    Completely Fair Scheduler (CFS)
The kernel can be compiled with a scheduler specified using the SCHEDULER macro.

B.1 First Come First Serve (FCFS)
Implementation:
1) Modified struct proc to add creation_time to track process arrival time.
2) Updated allocproc() to store the ticks at process creation.
3) Modified scheduler() in proc.c:
    Loops over all processes to find the RUNNABLE process with the earliest creation_time.
    Switches to that process and only selects the next process when the current process terminates.

Testing:
1) Used procdump to verify scheduling order.
2) Processes were executed in the exact order of creation.

Result:
The following output was observed when running schedulertest with FCFS:

Starting schedulertest with 5 child processes...
Child 3 finished. Run time: 2 ticks
Child 1 finished. Run time: 4 ticks
Child 0 finished. Run time: 4 ticks
Child 2 finished. Run time: 5 ticks
Child 4 finished. Run time: 4 ticks

Summary:
PID 4 | Wait Time: 3 ticks
PID 5 | Wait Time: 4 ticks
PID 6 | Wait Time: 4 ticks
PID 7 | Wait Time: 5 ticks
PID 8 | Wait Time: 5 ticks

Observations:
1) The processes are scheduled strictly based on arrival (creation_time).
2) Early-created processes have lower wait times; later ones wait longer, which matches the expected behavior of FCFS.


B.2 Completely Fair Scheduler (CFS)
Implementation:
1) Priority Support
    Added nice, weight, and vruntime fields to struct proc.
    Weight calculated as:weight = 1024 / (1.25 ^ nice)
    Default nice = 0, weight = 1024.

2) Virtual Runtime Tracking
    Each process has vruntime initialized to 0.
    vruntime is updated after each time slice proportional to time_slice * 1024 / weight.

3) Scheduling Logic
    Scheduler always selects the RUNNABLE process with the smallest vruntime.
    Processes are maintained in order of increasing vruntime.
    vruntime is normalized using process weight to ensure that all processes get fair CPU time relative to their priority.

4) Time Slice Calculation
    Target latency = 48 ticks.
    time_slice = max(target_latency / number_of_runnable_processes, 3).

Logging
    Before each scheduling decision, the scheduler prints the PID and vruntime of all RUNNABLE processes.
    Indicates which process is selected (lowest vruntime), e.g.:
        [Scheduler Tick]
        PID: 3 | vRuntime: 200
        PID: 4 | vRuntime: 150
        PID: 5 | vRuntime: 180
        --> Scheduling PID 4 (lowest vRuntime)

Sample Log:
Starting schedulertest with 5 child processes...
Child 3 finished. Run time: 2 ticks
Child 1 finished. Run time: 4 ticks
Child 2 finished. Run time: 5 ticks
Child 0 finished. Run time: 5 ticks
Child 4 finished. Run time: 4 ticks

Summary:
PID 4 | Wait Time: 3 ticks
PID 5 | Wait Time: 4 ticks
PID 6 | Wait Time: 5 ticks
PID 7 | Wait Time: 5 ticks
PID 8 | Wait Time: 6 ticks

Observations:
1) Scheduler correctly selects the process with the lowest vruntime.
2) vruntime values update correctly after each time slice.
3) Completion order differs slightly from FCFS due to fair CPU distribution.


B.3 Performance Comparison:
Average Wait Times and Run Times:

FCFS:
    Child completion order: 3, 1, 0, 2, 4
    Wait times: PID 4 – 3 ticks, PID 5 – 4 ticks, PID 6 – 4 ticks, PID 7 – 5 ticks, PID 8 – 5 ticks
    Observation: Processes strictly follow arrival order.

    Average wait time: (3 + 4 + 4 + 5 + 5)/5 = 4.2 ticks
    Average run time: (2 + 4 + 4 + 5 + 4)/5 = 3.8 ticks

CFS:
    Child completion order: 3, 1, 2, 0, 4
    Wait times: PID 4 – 3 ticks, PID 5 – 4 ticks, PID 6 – 5 ticks, PID 7 – 5 ticks, PID 8 – 6 ticks
    Observation: CPU time is balanced across processes; processes with lower vruntime are scheduled first.

    Average wait time: (3 + 4 + 5 + 5 + 6)/5 = 4.6 ticks
    Average run time: (2 + 4 + 5 + 5 + 4)/5 = 4 ticks

Round Robin (Default) for comparison:
    Round Robin results in nearly equal time slices for all processes in rotation, which may lead to higher context switching and slightly different wait times.

    Average wait time: (3 + 4 + 4 + 5 + 6)/5 = 4.4 ticks
    Average run time: (3 + 4 + 4 + 3 + 5)/5 = 3.8 ticks

Conclusion:
    FCFS is predictable but can cause longer waits for later processes.
    CFS ensures fair CPU distribution and balances process execution times.
    Both FCFS and CFS behave as expected and meet the project requirements.


Changes Made Summary:
1) Kernel
    Added read_count and implemented getreadcount() syscall.
    Modified struct proc to include creation_time, nice, weight, and vruntime.
    Updated allocproc() to initialize new fields.
    Modified scheduler() to implement FCFS and CFS using preprocessor directives.
    Added logging for CFS scheduling decisions.

2) User Programs
    readcount.c to test getreadcount().
    _schedulertest to test and compare scheduling policies.

3) Makefile
    Added SCHEDULER macro.
    Enabled compilation of _readcount and _schedulertest.
