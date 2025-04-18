# CPU Scheduler Simulator
A comprehensive simulator for CPU scheduling and memory allocation, demonstrating core operating system concepts through practical implementation.
## Project Overview
This simulator models how operating systems manage processes and allocate memory resources. It implements Round Robin CPU scheduling with three different memory management strategies:

Infinite Memory: Simulates an environment with no memory constraints
First-Fit Memory Allocation: Allocates the first available contiguous memory block that fits
Paged Memory Allocation: Divides memory into fixed-size frames with page replacement

## Features
### CPU Scheduling

Round Robin Algorithm: Processes execute for a fixed time quantum before potential preemption
Process States: Manages processes through READY, RUNNING, and FINISHED states
Dynamic Queue Management: Handles process arrivals and execution based on simulation time

### Memory Management

First-Fit Allocation: Implements contiguous memory allocation with the first-fit strategy
Paged Memory: Divides memory into fixed-size pages and frames
LRU Page Replacement: Evicts pages from least recently used processes when memory is full

### Performance Metrics

Turnaround Time: Average time from arrival to completion
Time Overhead: Ratio of turnaround time to service time (maximum and average)
Makespan: Total time to complete all processes

### How It Works

Processes are defined with arrival time, name, service time, and memory requirements
The simulator reads processes from an input file
Based on the chosen memory strategy, processes are allocated memory and scheduled for execution
The simulator outputs detailed execution logs showing state changes and memory allocation
Final performance metrics are calculated and displayed

### Usage
bash./scheduler -f <process_file> -m <memory_strategy> -q <quantum>
Arguments:

-f: Input file with process specifications
-m: Memory allocation strategy (infinite, first-fit, paged)
-q: Time quantum for Round Robin scheduling (e.g., 1, 2, 3)

### Input File Format
Each line in the input file represents a process with the following format:
<arrival_time> <process_name> <service_time> <memory_requirement>
Example:
0 P1 5 128
2 P2 3 256
4 P3 8 192
### Output
The simulator provides detailed logs showing:

Process state changes (READY → RUNNING → FINISHED)
Memory allocation details
Page evictions (for paged memory)
Final performance statistics

### Technical Implementation
The implementation features dynamic memory management, linked list queue structures, and modular code organization that clearly separates the different scheduling and memory allocation strategies.

## Test Cases
./allocate -f cases/task1/spec.txt -q 1 -m infinite | diff - cases/task1/spec-q1.out
./allocate -f cases/task1/two-processes.txt -q 1 -m infinite | diff - cases/task1/two-processes-q1.out
./allocate -f cases/task1/two-processes.txt -q 3 -m infinite | diff - cases/task1/two-processes-q3.out

./allocate -f cases/task2/fill.txt -q 3 -m first-fit | diff - cases/task2/fill-q3.out
./allocate -f cases/task2/non-fit.txt -q 1 -m first-fit | diff - cases/task2/non-fit-q1.out
./allocate -f cases/task2/retake-left.txt -q 3 -m first-fit | diff - cases/task2/retake-left-q3.out
./allocate -f cases/task2/consecutive-running.txt -q 3 -m first-fit | diff - cases/task2/consecutive-running-q3.out

./allocate -f cases/task3/simple-alloc.txt -q 3 -m paged | diff - cases/task3/simple-alloc-q3.out
./allocate -f cases/task3/simple-evict.txt -q 1 -m paged | diff - cases/task3/simple-evict-q1.out
./allocate -f cases/task3/internal-frag.txt -q 1 -m paged | diff - cases/task3/internal-frag-q1.out

./allocate -f cases/task4/no-evict.txt -q 3 -m virtual | diff - cases/task4/no-evict-q3.out
./allocate -f cases/task4/virtual-evict.txt -q 1 -m virtual | diff - cases/task4/virtual-evict-q1.out
./allocate -f cases/task4/virtual-evict-alt.txt -q 1 -m virtual | diff - cases/task4/virtual-evict-alt-q1.out
./allocate -f cases/task4/to-evict.txt -q 3 -m virtual | diff - cases/task4/to-evict-q3.out

./allocate -f cases/task1/spec.txt -q 1 -m infinite | diff - cases/task1/spec-q1.out
./allocate -f cases/task2/non-fit.txt -q 3 -m first-fit | diff - cases/task2/non-fit-q3.out
./allocate -f cases/task3/simple-alloc.txt -q 3 -m paged | diff - cases/task3/simple-alloc-q3.out
./allocate -f cases/task4/to-evict.txt -q 3 -m virtual | diff - cases/task4/to-evict-q3.out
