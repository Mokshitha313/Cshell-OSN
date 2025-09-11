[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/fkrRRp25)

Overview:
This repository contains mini-projects for Operating Systems and Networking coursework. The projects demonstrate practical implementation of system calls, process scheduling, shell functionality, and client-server communication using C and POSIX APIs.

Projects:
1) C Shell (Custom Shell):
    1) A custom shell implemented in C that supports:
        Executing commands
        Pipes (|) and redirection (>, <)
        Built-in commands like cd, exit, echo
        Background execution (&)
        Error handling for invalid commands

    2) Key Features:
        Tokenizes user input and executes commands with proper fork/exec handling.
        Supports both single commands and pipelines.
        Simple and modular design using separate .c and .h files.


2) Networking Mini-Project:
A TCP-based client-server chat and file transfer system in C.

Features:
    Secure and reliable message transfer
    File transfer with checksum verification
    Graceful termination of chat sessions
    Logging of events with timestamps

Client-Server Communication:
    Server listens on a specified port.
    Clients connect and exchange messages.
    Chat sessions terminate properly when either side exits.
    Supports multiple clients sequentially.


3) XV6 Mini Projects:
Part A: getreadcount System Call
    Tracks the total number of bytes read using read() across all processes since boot.
        Objective: Understand system call implementation in XV6.
        Implementation: Added a kernel-level counter, new system call, and updated syscall table.

Part B: Custom Scheduling Algorithms
    Implemented and tested:
        FCFS (First Come First Serve)
        CFS (Completely Fair Scheduler)
        Priority Scheduling

Features:
    Track process execution and waiting times.
    Compare different scheduling strategies.
    Extend XV6 kernel to include scheduler selection.

Usage:
    Boot XV6 in QEMU.
    Test system call and scheduling algorithms using included test programs.


Notes
    All projects use POSIX-compliant C with minimal external libraries.
    Care is taken to maintain modular, readable, and maintainable code.
    Known limitations and TODOs are documented within each project folder.

    
