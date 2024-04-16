#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/*******************************************************************************************************/
// List of Constants

/* Standard True/False Flags*/
#define TRUE 1
#define FALSE 0

/* All process-names will be distinct uppercase alphanumeric strings 
    of minimum length 1 and maximum length 8. (+1 for space)*/
#define MAX_PROCESS_NAME_LEN 9 
#define MAX_FILENAME_STRATEGY_LEN 100

/* Initial size of dynamic array storing processes */
#define INITIAL_PROCESSES 2

/* Defined states of a process */
#define READY 0
#define RUNNING 1
#define FINISHED 2

/*Memory strategies*/
#define INFINITE_MEMORY "infinite"
#define FIRST_FIT "first-fit"
#define PAGED "paged"
#define VIRTUAL "virtual"

#define MEMORY_CAPACITY 2048 // Total Memory in KB
#define NOT_ALLOCATED -1
#define FREE 0
#define ALLOCATED 1

#define SWITCH 1
#define CONTINUE 0

/*******************************************************************************************************/
/* Process Structure Definition 
*/
typedef struct {
    int arrivalTime;
    char processName[MAX_PROCESS_NAME_LEN];
    int serviceTime;
    int remainingTime;
    int memoryRequirement;
    int state;
    int cpuTimeUsed;
    int FFmemoryAllocation;
    int completionTime;
} Process;

/* Process Node
    to store the processes in a linked list 
*/
typedef struct ProcessNode{
    Process* process;
    struct ProcessNode* next;
} ProcessNode;

typedef struct ProcessQueue {
    struct ProcessNode *head, *tail;
} ProcessQueue;

/*******************************************************************************************************/
// List of Functions

/* Standard Queue Functions */
ProcessNode* newProcessNode(Process* process);
ProcessQueue* createQueue();
void enqueue(ProcessQueue* processQueue, Process* process);
Process* dequeue(ProcessQueue* processQueue);

/* Process Manager Functions */
void allocate(Process* processes, int processCount, int quantum, char memoryStrategy[]);
void readInput(int argc, char* argv[], char filename[], char memoryStrategy[], int* quantum);
Process* readProcesses(char filename[], int* processCount);
int* CreateMemoryBlock();
void checkProcesses(ProcessQueue* processQ, Process* processes, int processCount, int time, int* remaining, int quantum);
int update(Process* CPUproc, int quantum, int* time, int* finished, int remaining);
void calculateStatistics(Process* processes, int processCount);

void firstFitRR(ProcessQueue* processQ, int* memory, Process* processes, int processCount, int quantum);
void infiniteRR(ProcessQueue* processQ, Process* processes, int processCount, int quantum);

/*******************************************************************************************************/
int main(int argc, char* argv[]) {
    char filename[MAX_FILENAME_STRATEGY_LEN], memoryStrategy[MAX_FILENAME_STRATEGY_LEN];
    int quantum;
    
    /* read command line arguments for simulation specifications */
    readInput(argc, argv, filename, memoryStrategy, &quantum);

    /* read and store processes to be simulated */
    int processCount;
    Process* processes = readProcesses(filename, &processCount);
    for (int i = 0; i < processCount; i++) {
        printf("Process Name: %s, Arrival: %d, ServiceT: %d, RemainingT: %d, Memory: %d, State: %d, CPU: %d\n",
                processes[i].processName, processes[i].arrivalTime, processes[i].serviceTime, processes[i].remainingTime,
                processes[i].memoryRequirement, processes[i].state, processes[i].cpuTimeUsed);
    }

    /* allocate the processes for the CPU */
    allocate(processes, processCount, quantum, memoryStrategy);

    return 0;
}

/* process manager: allocate processes into the CPU until all completed;
    Acts as a queue, where only process at the head is considered RUNNING in the CPU
    and all following processes are READY
*/
void allocate(Process* processes, int processCount, int quantum, char memoryStrategy[]) {
    ProcessQueue* processQueue = createQueue();
    
    /* Task 1: Round robin with Infinite Memory */
    if(strcmp(INFINITE_MEMORY, memoryStrategy)== 0){
        infiniteRR(processQueue, processes, processCount, quantum);
    }
    else if (strcmp(memoryStrategy, FIRST_FIT) == 0) {
        printf("RUNNING FIRST FIT!\n");
        int* memory = CreateMemoryBlock();
        firstFitRR(processQueue, memory, processes, processCount, quantum);
    }  
    calculateStatistics(processes,processCount);
}
  

void calculateStatistics(Process* processes, int processCount){
    int total_time_turnaround = 0, avg_time_turnaround = 0;
    double total_time_overhead = 0, max_time_overhead = 0, avg_time_overhead;
    int makespan = 0;
    for(int i = 0; i<processCount; i++){
        
        // Turnaround Time
        total_time_turnaround += processes[i].completionTime - processes[i].arrivalTime;
        printf("%d\n",processes[i].completionTime - processes[i].arrivalTime);

        // Time Overhead
        double time_overhead = (double)(processes[i].completionTime - processes[i].arrivalTime)/processes[i].serviceTime;
        total_time_overhead += time_overhead;
        if(max_time_overhead<time_overhead){
            max_time_overhead = time_overhead;
        }

        //Makespan
        if(i == processCount-1){
            makespan = processes[i].completionTime;
        }
        
    }

    max_time_overhead = max_time_overhead;
    avg_time_turnaround = ceil((double)total_time_turnaround/processCount);
    avg_time_overhead = total_time_overhead/processCount;

    printf("Turnaround Time %d\n",avg_time_turnaround);
    printf("Time overhead %.2f %.2f\n", max_time_overhead,avg_time_overhead );
    printf("Makespan %d", makespan);
}

/* Round-Robin Scheduling with First-Fit Memory Allocation
*/
void firstFitRR(ProcessQueue* processQ, int* memory, Process* processes, int processCount, int quantum) {
    int time, finished, remaining, turnaroundT;
    time = finished = remaining = turnaroundT = 0;

    /* loop will run until all processes are FINISHED*/
    while (finished < processCount) {
    
        if (time == 25) {
            break;
        }
        /* check if any new processes need to added to the queue */
        checkProcesses(processQ, processes, processCount, time, &remaining, quantum);
        
        /* take the process at the head of the queue,
            this process is now considered in the CPU */
        Process* CPUproc;
        if (processQ->head != NULL) {
            CPUproc = (processQ->head->process);
        } else {
            printf("t%d No Processes Queued\n", time);
            time++;
            continue;
        }

        /* before running a process in CPU, check memory allocation
            if not allocated, allocate memory */
        if (CPUproc->FFmemoryAllocation == NOT_ALLOCATED) {
            printf("process %s needs memory\n", CPUproc->processName);
            /* scan through memory block to check for a fit */
            int freeBlockSize = 0;
            for (int i = 0; i < MEMORY_CAPACITY; i++) {
                if (memory[i] == FREE) {
                    freeBlockSize++;
                    /* if there is space, mark the start index of memory block */
                    if (freeBlockSize == CPUproc->memoryRequirement) {
                        CPUproc->FFmemoryAllocation = i - CPUproc->memoryRequirement + 1;
                        /* mark the block as allocated memory */
                        for (int j = CPUproc->FFmemoryAllocation; j <= i; j++) {
                            memory[j] = ALLOCATED;
                        }
                        printf("Memory Block Allocated! Process: %s, Allocation: %d, end: %d\n", 
                                        CPUproc->processName, CPUproc->FFmemoryAllocation, (CPUproc->FFmemoryAllocation + CPUproc->memoryRequirement - 1));
                    }
                /* if a memory space is already ALLOCATED and freeBlocksize < memoryREQ,
                    reset search */
                } else {
                    freeBlockSize = 0;
                }
            }
        }
        /* if memory is still not allocated, deque process and move to back of the queue */
        if (CPUproc->FFmemoryAllocation == NOT_ALLOCATED) {
            printf("MEMORY BLOCK NOT ALLOCATED!\n");
            Process* sendBack = dequeue(processQ);
            enqueue(processQ, sendBack);
        }

        /* run the process in the CPU for a quantum, as usual */
        else {
            int instruction = update(CPUproc, quantum, &time, &finished, remaining);
            /* if there are other processes in the queue at the start of a quantum, SWITCH.
                send current process to back of the queue and run a new process in the CPU */
            while (instruction != CONTINUE) {
                printf("SWITCH PROCESS!\n");
                Process* sendBack = dequeue(processQ);
                if (sendBack->state == FINISHED) {
                    printf("%d PROCESS %s FINISHED!\n", time, sendBack->processName);
                    remaining--;
                } else {
                    sendBack->state = READY;
                    enqueue(processQ, sendBack);
                }

                /* continue as idlle if no other processes are queued*/
                if (remaining == 0) {
                    break;
                } else {
                    /* put a new process into the CPU and run again */
                    CPUproc = processQ->head->process;
                    instruction = update(CPUproc, quantum, &time, &finished, remaining);
                }
            }    
        }

    time++;
    }
}
    
/* Allocation of a contiguous memory block;
    memory is treated as an integer array of size 2048 KB and each element is 1KB of memory,
    where 0 indicates a free spot and 1 indicates an allocated space
*/
int* CreateMemoryBlock() {
    int* memory = (int*) calloc(MEMORY_CAPACITY, sizeof(int));
    if (memory == NULL) {
        fprintf(stderr, "Failed to allocate memory block\n");
        exit(EXIT_FAILURE);
    }
    return memory;
}

/* Round-Robin Scheduling with Infinite Memory */
void infiniteRR(ProcessQueue* processQ, Process* processes, int processCount, int quantum) {
    int time, finished, remaining, turnaroundTime;
    time = finished = remaining = turnaroundTime = 0;

    while (finished < processCount) {

        /* check if any new processes need to added to the queue */
        checkProcesses(processQ, processes, processCount, time, &remaining,quantum);

        /* take the process at the head of the queue,
            this process is now considered in the CPU */
        Process* CPUproc;
        if (processQ->head != NULL) {
            CPUproc = (processQ->head->process);
        } else {
            time += quantum;
            continue;
        }

        int instruction = update(CPUproc, quantum, &time, &finished, remaining);
        /* if there are other processes in the queue at the start of a quantum, SWITCH.
            send current process to back of the queue and run a new process in the CPU */
        while (instruction != CONTINUE) {
            Process* sendBack = dequeue(processQ);

            /* if process is FINISHED, dequue, print, and decrement remainingProcesses*/
            if (sendBack->state == FINISHED) {
                remaining--;
                printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", time, sendBack->processName, remaining);
                sendBack->completionTime = time;
            
                /* continue as idlle if no other processes are queued*/
                if (remaining == 0) {
                    break;
                }

            /* if instructions is switch, and there are other processes READY,
                 send to ready */
            } else if (sendBack->state == SWITCH) {
                if (remaining > 1) {
                    sendBack->state = READY;
                } 
                /* BUT if it is the only processs, enqueue in RUNNING state */
                enqueue(processQ, sendBack);
            }

            /* put a new process into the CPU and run again */
            CPUproc = processQ->head->process;
            instruction = update(CPUproc, quantum, &time, &finished, remaining);
        } 

        time += quantum;
    }

}

/* standard update function. Runs the CPU process for ONE time unit
*/
int update(Process* CPUproc, int quantum, int* time, int* finished, int remaining) {
    /* check whether the current CPU process has just entered, 
        or if it was already running */
    int isNew = FALSE, remainingTime;
    if (CPUproc->state == READY) {
        CPUproc->state = RUNNING;
        /* mark the process as NEW as up to this point, 
            the current CPU process was only considered a candidate */
        isNew = TRUE;
        remainingTime = CPUproc->serviceTime - CPUproc->cpuTimeUsed;
        printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", *time, CPUproc->processName, remainingTime);
    }

    /* check if quantum has elapsed, since process switching and completion
        can ONLY be performed at the end of a quantum*/
    if ((CPUproc->state == RUNNING) && (!isNew)) {
        if (CPUproc->cpuTimeUsed >= CPUproc->serviceTime) {
            CPUproc->state = FINISHED;
            (*finished)++;
            return FINISHED;
        } 
        /* SWITCH only if there are other processes READY waiting */
        else if (remaining > 1) { 
            return SWITCH;
        } 
    }

    /* run the process for ONE time unit */
    if ((CPUproc->state == RUNNING)) {
        /* increment CPU time used */
        CPUproc->cpuTimeUsed += quantum;  
    }

    return CONTINUE;
} 

/* Check for READY processes according to arrival time
*/
void checkProcesses(ProcessQueue* processQ, Process* processes, int processCount, int time, int* remaining, int quantum) {

    /* check if any processes are ready to enque based on arrival time */
    for (int i = 0; i < processCount; i++) {
        if (processes[i].arrivalTime == time || (processes[i].arrivalTime >= time && processes[i].arrivalTime < (time-quantum))) {
            enqueue(processQ, &processes[i]);
            (*remaining)++;
        }
    }
}  

/*******************************************************************************************************/
// Basic pre-task HELPER FUNCTIONS
/* read command line arguments to deterimine:
    list of processes, memory strategy, & quantum length
*/
void readInput(int argc, char* argv[], char filename[], char memoryStrategy[], int* quantum){
    for (int i = 1; i < argc - 1; i++) {
        /* filename */
        if (strcmp(argv[i], "-f") == 0) {
            strcpy(filename, argv[++i]);
        /* memory strategy (infinite, first-fit, paged, virtual)*/
        } else if (strcmp(argv[i], "-m") == 0) {
            strcpy(memoryStrategy, argv[++i]);
        /* quantum length (1, 2, 3)*/
        } else if (strcmp(argv[i], "-q") == 0) {
            *quantum = atoi(argv[++i]);
        }
    }
}

/* read list of processes from file and store their info
*/
Process* readProcesses(char filename[], int* processCount) {
    /* open file in read, exit if not found */
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Initialization of dynamic array to store processes */
    size_t curentSize = INITIAL_PROCESSES;
    Process *processes = malloc(sizeof(Process) * curentSize);
    if (processes == NULL) {
        fprintf(stderr, "Malloc failure: Process Array Not Initialized\n");
        exit(EXIT_FAILURE);
    }
    *processCount = 0;

    /* reading processes into the array */
    int arrival, serviceTime, memoryReq, remainingTime;
    char name[MAX_PROCESS_NAME_LEN];
    while (fscanf(fp, "%d %s %d %d", &arrival, &name, &serviceTime, &memoryReq) == 4) {
        /* reallocate memory if needed */
        if (*processCount == curentSize) {
            curentSize *= 2;
            processes = realloc(processes, sizeof(Process) * curentSize);
            if (processes == NULL) {
                fprintf(stderr, "Realloc failure: Process Array not reallocated in memory\n");
                exit(EXIT_FAILURE);
            }
        }
        
        /* Initialize process structure */
        processes[*processCount].arrivalTime = arrival;
        strcpy(processes[*processCount].processName, name);
        processes[*processCount].serviceTime = serviceTime;
        processes[*processCount].remainingTime = serviceTime;
        processes[*processCount].memoryRequirement = memoryReq;
        processes[*processCount].state = READY;
        processes[*processCount].cpuTimeUsed = READY;
        processes[*processCount].FFmemoryAllocation = NOT_ALLOCATED;
        processes[*processCount].completionTime = 0;
        (*processCount)++;
    }

    fclose(fp);
    return processes;
}

// Process Queue Functions
ProcessNode* newProcessNode(Process* process) {
    ProcessNode* temp = (ProcessNode*)malloc(sizeof(ProcessNode));
    temp->process = process;
    temp->next = NULL;
    return temp;
}

ProcessQueue* createQueue() {
    ProcessQueue* q = (ProcessQueue*)malloc(sizeof(ProcessQueue));
    q->head = q->tail = NULL;
    return q;
}

/* place a process at the end of the CPU queue
*/
void enqueue(ProcessQueue* processQueue, Process* process) {
    ProcessNode* temp = newProcessNode(process);
    if (processQueue->tail == NULL) {
        processQueue->head = processQueue->tail = temp;
        return;
    }
    processQueue->tail->next = temp;
    processQueue->tail = temp;
}
/* take the process currently running in the CPU
*/
Process* dequeue(ProcessQueue* processQueue) {
    if (processQueue->head == NULL) {
        printf("Queue Empty\n");
        exit(EXIT_FAILURE);
    }
    ProcessNode* temp = processQueue->head;
    Process* process = temp->process;
    processQueue->head = processQueue->head->next;
    if (processQueue->head == NULL) {
        processQueue->tail = NULL;
    }
    free(temp);
    return process;
}