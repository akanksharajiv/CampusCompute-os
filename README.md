# CampusCompute OS Mini Project

**CampusCompute: Intelligent Job Scheduling and Resource Management**

CampusCompute is a C-based operating system simulation developed to demonstrate fundamental OS concepts through practical implementation. The project brings together CPU scheduling, concurrent execution, inter-process communication, resource allocation, process monitoring, and resource management in a unified command-line application.

## Project Overview

The project simulates a shared computing environment in which multiple jobs compete for processing time and system resources. It demonstrates how operating systems coordinate processes, manage shared resources, handle synchronization, and maintain reliable execution.

The application is implemented in C and designed to run on Linux/Ubuntu.

## Features

The project implements the following functional requirements:

### FR1 – Job Management and Process Lifecycle
- Job creation and management.
- Input validation and job status tracking.
- Monitoring job execution and completion.

### FR2 – CPU Scheduling Algorithms
- First-Come, First-Served (FCFS).
- Round Robin scheduling.
- Non-preemptive Priority scheduling.
- Execution order and scheduling metrics, including completion time, turnaround time, and waiting time.

### FR3 – Concurrent Activity Logger
- Producer-consumer implementation using POSIX threads.
- Bounded queue for event management.
- Thread synchronization using mutexes and condition variables.
- Event logging and clean thread termination.

### FR4 – Inter-Process Communication
- POSIX shared memory for communication between cooperating processes.
- Named semaphore synchronization.
- Error handling and IPC resource cleanup.

### FR5 – Banker's Algorithm
- Resource allocation and safety analysis.
- Calculation of the Need matrix.
- Identification of safe and unsafe states.
- Safe-sequence determination.

### FR6 – Worker Process Management
- Child process creation using `fork()`.
- Process completion monitoring using `waitpid()`.
- Normal worker execution and controlled simulated failure detection.

### FR7 – Resource Limits and Cleanup
- Process resource limits using `setrlimit()`.
- Monitoring resource-limit behaviour.
- Cleanup of program-created processes, threads, files, shared memory, and semaphores.

### FR8 – Command-Line Interface and Error Handling
- Command-based access to project features.
- Input validation and meaningful error messages.
- Independent feature execution and integrated demonstration.

## Technologies Used

- **Programming Language:** C
- **Operating System:** Linux / Ubuntu
- **Process Management:** `fork()`, `waitpid()`
- **Concurrency:** POSIX Threads (Pthreads)
- **Inter-Process Communication:** POSIX Shared Memory and Semaphores
- **Resource Management:** `setrlimit()`
- **Build System:** GNU Make
- **Version Control:** Git and GitHub

## Project Structure

The project is organized into source files, header files, and tests, with a Makefile for building the application.

```text
CampusCompute/
├── include/
├── src/
│   ├── main.c
│   ├── jobs.c
│   ├── scheduler.c
│   ├── logger.c
│   ├── ipc.c
│   ├── banker.c
│   ├── workers.c
│   └── demo.c
├── tests/
├── Makefile
├── README.md
└── .gitignore
```

## Build and Execution

### Prerequisites

- Linux / Ubuntu environment
- GCC compiler
- GNU Make
- POSIX threads and shared-memory support

### Compile the Project

```bash
make
```

### Run the Integrated Demonstration

```bash
./campuscompute demo
```

### Run Individual Features

Use the application's command-line interface to access the supported modules and demonstrations.

### Clean Build Files

```bash
make clean
```

## Testing and Verification

The project is verified through individual feature demonstrations and integrated execution.

Testing covers:

- CPU scheduling and calculated performance metrics.
- Producer-consumer synchronization and event logging.
- Shared-memory communication and semaphore coordination.
- Safe and unsafe resource-allocation scenarios.
- Normal and simulated worker failures.
- Resource-limit behaviour.
- Invalid command handling and error reporting.
- Resource cleanup and integrated execution.

## Team Contributions

**Person A**
- Job management and process lifecycle (FR1).
- CPU scheduling algorithms (FR2).
- Concurrent activity logger (FR3).
- Main application and command-line integration.
- Build configuration and Makefile.

**Person B**
- Inter-process communication (FR4).
- Banker's resource-safety algorithm (FR5).
- Worker process management (FR6).
- Resource limits and cleanup (FR7).

Both team members contributed to integration, verification, and testing of the complete application.

## Project Information

**Project:** CampusCompute – Intelligent Job Scheduling and Resource Management  
**Course:** Operating Systems (UE25CS242A)  
**Project Type:** Level 3 Mini Project  
**Language:** C  
**Platform:** Linux / Ubuntu

---

*Developed as part of the Operating Systems Level 3 Mini Project.*
