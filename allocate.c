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

/* Process Manager strategies*/
#define INFINITE_MEMORY "infinite"
#define FIRST_FIT "first-fit"
#define PAGED "paged"
#define VIRTUAL "virtual"

/* Memory Constants and Flags */
#define MEMORY_CAPACITY 2048 // Total Memory in KB
#define NUM_PAGES 512
#define PAGE_SIZE 4
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
    int memoryRequirement;
    int state;
    int cpuTimeUsed;
    int FFmemoryAllocation;
    int* PmemoryAllocation;
    int sizeOfFrames;
    int lastUsed;
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

/* Pre-Task Process Functions */
void readInput(int argc, char* argv[], char filename[], char memoryStrategy[], int* quantum);
Process* readProcesses(char filename[], int* processCount);

/* Process Manager Functions */
void allocate(Process* processes, int processCount, int quantum, char memoryStrategy[]);
void checkProcesses(ProcessQueue* processQ, Process* processes, int processCount, 
                    int time, int* remaining, int quantum);
int update(Process* CPUproc, int quantum, int* time, int* finished, int remaining, int* isNew);
void calculateStatistics(Process* processes, int processCount);

int calculateMemUsage(int* memory);
int calculatePageMemUsage(int* pages);
int* createMemory(char memoryStrategy[]);
int allocateMemoryBlock(int* memory, int memoryRequirement);
int* allocatePages(int* memory, int memoryRequirement, int* frameSize);
void deallocateMemoryBlock(int* memory, int allocationStart, int allocationSize);
void deallocatePages(int* memory, int* frameSize, int* frames);
void evictLRU(int processCount, Process* processes, int* memory, int time);

/* Task Algorithms */
void firstFitRR(ProcessQueue* processQ, int* memory, Process* processes, int processCount, int quantum);
void infiniteRR(ProcessQueue* processQ, Process* processes, int processCount, int quantum);
void pagedMemoryRR(ProcessQueue* processQ, int* pages, Process* processes, int processCount, int quantum);

/*******************************************************************************************************/
int main(int argc, char* argv[]) {
    char filename[MAX_FILENAME_STRATEGY_LEN], memoryStrategy[MAX_FILENAME_STRATEGY_LEN];
    int quantum;
    
    /* read command line arguments for simulation specifications */
    readInput(argc, argv, filename, memoryStrategy, &quantum);

    /* read and store processes to be simulated */
    int processCount;
    Process* processes = readProcesses(filename, &processCount);

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
        int* memory = createMemory(memoryStrategy);
        firstFitRR(processQueue, memory, processes, processCount, quantum);
    }  
    else if (strcmp(memoryStrategy, PAGED) == 0) {
        int* pages = createMemory(memoryStrategy);
        pagedMemoryRR(processQueue, pages, processes, processCount, quantum);
        
    }
    calculateStatistics(processes,processCount);

    free(processQueue);
    free(processes);

}
  

void calculateStatistics(Process* processes, int processCount){
    int total_time_turnaround = 0, avg_time_turnaround = 0;
    double total_time_overhead = 0, max_time_overhead = 0, avg_time_overhead;
    int makespan = 0;
    for(int i = 0; i<processCount; i++){
        
        // Turnaround Time
        total_time_turnaround += processes[i].completionTime - processes[i].arrivalTime;
        

        // Time Overhead
        double time_overhead = (double)(processes[i].completionTime - 
                                processes[i].arrivalTime)/processes[i].serviceTime;
        total_time_overhead += time_overhead;
        if(max_time_overhead<time_overhead){
            max_time_overhead = time_overhead;
        }

        //Makespan
        if(makespan < processes[i].completionTime){
            makespan = processes[i].completionTime;
        
        }
        
    }

    max_time_overhead = round(max_time_overhead*100)/100;
    avg_time_turnaround = ceil((double)total_time_turnaround/processCount);
    avg_time_overhead = round(total_time_overhead/processCount*100)/100;

    printf("Turnaround time %d\n",avg_time_turnaround);
    printf("Time overhead %.2f %.2f\n", max_time_overhead,avg_time_overhead );
    printf("Makespan %d", makespan);
}

/*******************************************************************************************************/
/*Round-Robin Scheduling with Paged Memory Allocation
*/
void pagedMemoryRR(ProcessQueue* processQ, int* pages, Process* processes, int processCount, int quantum){
    int time, finished, remaining;
    time = finished = remaining = 0;

    while (finished < processCount){
        // Checks if a new process can be added to the queue
        checkProcesses(processQ, processes, processCount, time, &remaining, quantum);

        /*Takes the process at the head of the queue*/
        Process* CPUproc;
        if (processQ->head != NULL) {
            CPUproc = (processQ->head->process);
        } else {
            //printf("t%d No Processes Queued\n", time);
            time += quantum;
            continue;
        }
        /* before running a process in CPU, check if it has allocated memory */
        if (CPUproc->PmemoryAllocation == NULL || CPUproc->PmemoryAllocation[0] == NOT_ALLOCATED) {
            CPUproc->PmemoryAllocation = allocatePages(pages, CPUproc->memoryRequirement, &CPUproc->sizeOfFrames);

            /* if memory is still not allocated, evict least recently used pages of a processor
               until enough memory is available */
            if (CPUproc->PmemoryAllocation[0] == NOT_ALLOCATED) {
                    while(CPUproc->PmemoryAllocation[0] == NOT_ALLOCATED){
                        evictLRU(processCount,processes,pages,time);
                        CPUproc->PmemoryAllocation = allocatePages(pages, CPUproc->memoryRequirement, 
                                                                    &CPUproc->sizeOfFrames);
                    }
            }
        }
      
        /* isNew flag determines whether the process in the CPU was switched from READY to RUNNING */
        int isNew = FALSE;

        /* this is the first instruction call. It will determine what action to take for the quantum.
            the instruction is either CONTINUE, FINISHED, or SWITCH*/
        int instruction = update(CPUproc, quantum, &time, &finished, remaining, &isNew);

        /* if the process was NEW in the CPU, print out a message */
        if (isNew) {
            
            int  remainingTime = CPUproc->serviceTime - CPUproc->cpuTimeUsed;
            
            int memUsage = calculatePageMemUsage(pages);
           
            printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,mem-frames=[", 
                    /* decrement time in message in accordance with qunatum */
                    (time - quantum), CPUproc->processName, (remainingTime + quantum), 
                        memUsage);
            CPUproc->lastUsed = time-quantum;

            for(int i=0; i<CPUproc->sizeOfFrames;i++){
                printf("%d",CPUproc->PmemoryAllocation[i]);
                if(i<CPUproc->sizeOfFrames - 1){
                    printf(",");
                }
            }

            printf("]\n");
        }

        /* a CONTINUE call indicates that update() allowed the current CPUproc to run for a quantum.
            if not, there is a call to switch out the current CPUproc
            or that the process has FINISHED at the end of the quantum */
        while (instruction != CONTINUE) {
            Process* sendBack = dequeue(processQ);

            /* if process is FINISHED, its serviceTime has been completed
                and it can be removed from the queue */
            if (sendBack->state == FINISHED) {
                /* free the memory block allocated for the process */
                printf("%d,EVICTED,evicted-frames=[",time);
                for(int i=0; i<sendBack->sizeOfFrames;i++){
                    printf("%d",sendBack->PmemoryAllocation[i]);
                    if(i<sendBack->sizeOfFrames - 1){
                        printf(",");
                    }
                }
                    printf("]\n");

                deallocatePages(pages, &sendBack->sizeOfFrames, sendBack->PmemoryAllocation);

                remaining--;
                printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", time, sendBack->processName, remaining);
                sendBack->completionTime = time;
        
                /* continue as idle if no other processes are queued*/
                if (remaining == 0) {
                    break;
                }

            /* if instructions is SWITCH, 
                which means that the current process has already run the last quantum, 
                AND there are also other processes READY,
                the CPU will switch out the process and send the current one to the back. */
            } else if (instruction == SWITCH) {
                sendBack->lastUsed = time;
                sendBack->state = READY;
                enqueue(processQ, sendBack);
            } 

            /* select the new process to run in the CPU */
            CPUproc = processQ->head->process;  

            /* in the case that a NEW process is in the CPU, but it has not been allocated memory.. */
            if (CPUproc->PmemoryAllocation == NULL || CPUproc->PmemoryAllocation[0] == NOT_ALLOCATED) {
                CPUproc->PmemoryAllocation = allocatePages(pages, CPUproc->memoryRequirement, &CPUproc->sizeOfFrames);

            /* if memory is still not allocated, evict least recently used pages of a processor
               until there is enough memory for the current processor */
                if (CPUproc->PmemoryAllocation[0] == NOT_ALLOCATED) {
                    while(CPUproc->PmemoryAllocation[0] == NOT_ALLOCATED){
                        evictLRU(processCount,processes,pages,time);
                        CPUproc->PmemoryAllocation = allocatePages(pages, CPUproc->memoryRequirement, 
                                                                    &CPUproc->sizeOfFrames);
                    }
                }
            }

            /* This update call will only be done IF a new process is now in the CPU
                or the current process is allowed to continue, given no other ready processes, 
                AND the process has been allocated memory */
            instruction = update(CPUproc, quantum, &time, &finished, remaining, &isNew);

            /* if the process was NEW in the CPU, print out a message */
            if (isNew) {
                int  remainingTime = CPUproc->serviceTime - CPUproc->cpuTimeUsed;
                int memUsage = calculatePageMemUsage(pages);
           
                printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,mem-frames=[", 
                    /* decrement time in message in accordance with qunatum */
                    (time - quantum), CPUproc->processName, (remainingTime + quantum), 
                        memUsage);

            CPUproc->lastUsed = time-quantum;

                for(int i=0; i<CPUproc->sizeOfFrames;i++){
                    printf("%d",CPUproc->PmemoryAllocation[i]);
                    if(i<CPUproc->sizeOfFrames - 1){
                        printf(",");
                    }
                }
                printf("]\n");
            }
        }    
    }
    free(pages);
}

/* Round-Robin Scheduling with First-Fit Memory Allocation
*/
void firstFitRR(ProcessQueue* processQ, int* memory, Process* processes, int processCount, int quantum) {
    int time, finished, remaining;
    time = finished = remaining = 0;

    /* loop will run until all processes are FINISHED*/
    while (finished < processCount) {
    
        /* check if any new processes need to added to the queue */
        checkProcesses(processQ, processes, processCount, time, &remaining, quantum);
        
        /* take the process at the head of the queue,
            this process is now considered in the CPU */
        Process* CPUproc;
        if (processQ->head != NULL) {
            CPUproc = (processQ->head->process);
        } else {
            time += quantum;
            continue;
        }

        /* before running a process in CPU, check if has allocated memory */
        while (CPUproc->FFmemoryAllocation == NOT_ALLOCATED) {
            CPUproc->FFmemoryAllocation = allocateMemoryBlock(memory, CPUproc->memoryRequirement);

            /* if memory is still not allocated, deque process and move to back of the queue */
            if (CPUproc->FFmemoryAllocation == NOT_ALLOCATED) {
                Process* sendBack = dequeue(processQ);
                enqueue(processQ, sendBack);
                CPUproc = processQ->head->process;
            }
        }

        /* isNew flag determines whether the process in the CPU was switched from READY to RUNNING */
        int isNew = FALSE;

        /* this is the first instruction call. It will determine what action to take for the quantum.
            the instruction is either CONTINUE, FINISHED, or SWITCH*/
        int instruction = update(CPUproc, quantum, &time, &finished, remaining, &isNew);

        /* if the process was NEW in the CPU, print out a message */
        if (isNew) {
            int  remainingTime = CPUproc->serviceTime - CPUproc->cpuTimeUsed;
            int memUsage = calculateMemUsage(memory);
            printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n", 
                    /* decrement time in message in accordance with qunatum */
                    (time - quantum), CPUproc->processName, (remainingTime + quantum), 
                        memUsage, CPUproc->FFmemoryAllocation);
        }

        /* a CONTINUE call indicates that update() allowed the current CPUproc to run for a quantum.
            if not, there is a call to switch out the current CPUproc
            or that the process has FINISHED at the end of the quantum */
        while (instruction != CONTINUE) {
            Process* sendBack = dequeue(processQ);

            /* if process is FINISHED, its serviceTime has been completed
                and it can be removed from the queue */
            if (sendBack->state == FINISHED) {
                /* free the memory block allocated for the process */
                deallocateMemoryBlock(memory, sendBack->FFmemoryAllocation, sendBack->memoryRequirement);

                remaining--;
                printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", time, sendBack->processName, remaining);
                sendBack->completionTime = time;
        
                /* continue as idlle if no other processes are queued*/
                if (remaining == 0) {
                    break;
                }

            /* if instructions is SWITCH, 
                which means that the current process has already run the last quantum, 
                AND there are also other processes READY,
                the CPU will switch out the process and send the current one to the back. */
            } else if (instruction == SWITCH) {
                sendBack->state = READY;
                enqueue(processQ, sendBack);
            } 

            /* select the new process to run in the CPU */
            CPUproc = processQ->head->process;  

            /* in the case that a NEW process is in the CPU, but it has not been allocated memory.. */
            while (CPUproc->FFmemoryAllocation == NOT_ALLOCATED) {
                /* allocate memory for new process */
                CPUproc->FFmemoryAllocation = allocateMemoryBlock(memory, CPUproc->memoryRequirement);

                /* if memory is still not allocated, dequeue process and move to back of the queue,
                    continue until the current CPUproc has memory allocated, allowing it to RUN */
                if (CPUproc->FFmemoryAllocation == NOT_ALLOCATED) {
                    Process* sendBack = dequeue(processQ);
                    enqueue(processQ, sendBack);
                    CPUproc = processQ->head->process;
                } 
            }

            /* This update call will only be done IF a new process is now in the CPU
                or the current is allowed to continue, given no other ready processes, 
                AND the process has been allocated memory */
            instruction = update(CPUproc, quantum, &time, &finished, remaining, &isNew);

            /* if the process was NEW in the CPU, print out a message */
            if (isNew) {
                int  remainingTime = CPUproc->serviceTime - CPUproc->cpuTimeUsed;
                int memUsage = calculateMemUsage(memory);
                printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n", 
                        /* decrement time in message in accordance with quantum */
                        (time - quantum), CPUproc->processName, (remainingTime + quantum), 
                            memUsage, CPUproc->FFmemoryAllocation);
            }
        }    
    }
}

/*Evict pages of least recently used processor
*/
void evictLRU(int processCount, Process* processes, int* memory, int time){
    int least_recent = 0;
    for(int i=0; i<processCount; i++){
        if(processes[i].state != FINISHED && processes[i].lastUsed >= 0){
            if(processes[i].lastUsed < least_recent){
                least_recent = i;
            }
        }
    }
    printf("%d,EVICTED,evicted-frames=[",time);
    for(int i=0; i<processes[least_recent].sizeOfFrames;i++){
        printf("%d",processes[least_recent].PmemoryAllocation[i]);
        if(i<processes[least_recent].sizeOfFrames - 1){
            printf(",");
        }
    }
    printf("]\n");

    deallocatePages(memory,&processes[least_recent].sizeOfFrames,
        processes[least_recent].PmemoryAllocation);
}


/* calculate percentage of total memory used, rounded up
*/
int calculateMemUsage(int* memory) {
    double usage = 0;
    for (int i = 0; i < MEMORY_CAPACITY; i++) {
        if (memory[i] == ALLOCATED) {
            usage++;
        }
    }
    usage = ceil((usage / MEMORY_CAPACITY) * 100);

    return (int)usage;
}

int calculatePageMemUsage(int* pages) {
    double usage = 0;
    for (int i = 0; i < NUM_PAGES; i++) {
        if (pages[i] == ALLOCATED) {
            usage++;
        }
    }
    usage = ceil((usage / NUM_PAGES) * 100);

    return (int)usage;
}

/* Free the block of memory associate with a process 
*/
void deallocateMemoryBlock(int* memory, int allocationStart, int allocationSize) {
    for (int i = allocationStart; i < (allocationStart + allocationSize); i++) {
        memory[i] = FREE;
    }
}

/*Free the pages associate with a process */
void deallocatePages(int* memory, int* frameSize, int* frames){
    int free_page_number = 0;
    for(int i=0; i< *frameSize; i++){
        free_page_number = frames[i];
        memory[free_page_number] = FREE;
        frames[i] = NOT_ALLOCATED;
    }
}

/* Allocate a contiguous block of memory for a process
*/    
int allocateMemoryBlock(int* memory, int memoryRequirement) {
    
    int freeBlockSize = 0, memoryAllocation = NOT_ALLOCATED;

    /* scan through memory block to check for a fit */
    for (int i = 0; i < MEMORY_CAPACITY; i++) {
        if (memory[i] == FREE) {
            freeBlockSize++;
            /* if there is space, mark the start index of memory block */
            if (freeBlockSize == memoryRequirement) {
                memoryAllocation = i - memoryRequirement + 1;
                /* mark the block as allocated memory */
                for (int j = memoryAllocation; j <= i; j++) {
                    memory[j] = ALLOCATED;
                }
            }
        /* if a memory space is already ALLOCATED and freeBlocksize < memoryREQ,
            reset search */
        } else {
            freeBlockSize = 0;
        }
    }
    return memoryAllocation;
}
// Allocates pages for a process
int* allocatePages(int* memory, int memoryRequirement, int* frameSize) {

    int freeBlockSize = 0, memoryAllocation = NOT_ALLOCATED;
    int pagesRequired = 0;

    // Allocate memory into pages
    if(memoryRequirement % PAGE_SIZE == 0 ){
        pagesRequired = memoryRequirement/PAGE_SIZE;
    } else{
        pagesRequired = (memoryRequirement/PAGE_SIZE) + 1;
    }

    *frameSize = pagesRequired;
    int* frames = (int*) malloc(pagesRequired * sizeof(int));
    if (frames == NULL) {
        fprintf(stderr, "Failed to allocate memory block\n");
        exit(EXIT_FAILURE);
    }

    // Initialize the frames
    for(int i = 0; i< pagesRequired ; i++){
        frames[i] = NOT_ALLOCATED;
    }

    /* scan through memory block to check for a fit */
    for (int i = 0; i < NUM_PAGES; i++) {
        if (memory[i] == FREE) {
            freeBlockSize++;
            /* if there is space, mark the start index of memory block */
            if (freeBlockSize == pagesRequired) {
                memoryAllocation = i - pagesRequired + 1;
                /* mark the block as allocated memory */
                for (int j = memoryAllocation; j <= i; j++) {
                    if(memory[j] != ALLOCATED){
                        memory[j] = ALLOCATED;

                        // mark the frames used by the processor
                        for(int k=0; k<pagesRequired; k++){
                            if(frames[k] == NOT_ALLOCATED){
                                frames[k] = j;
                                //printf("%d,",j);
                                break;
                            }
                        }
                    }
                }
            }
        /* if a memory space is already ALLOCATED and freeBlocksize < memoryREQ,
            reset search */
        } else {
            freeBlockSize = 0;
        }
    }
    return frames;
}

/* Allocation of memory;
    memory is treated as an integer array of size 2048 KB 512KB and each element is 1KB of memory,
    OR integer array of size 512KB,
    where 0 indicates a free spot and 1 indicates an allocated space
*/
int* createMemory(char memoryStrategy[]) {
    int memorySize = 0;
    if(strcmp(memoryStrategy, FIRST_FIT) == 0){
        memorySize = MEMORY_CAPACITY;
    } else{
        memorySize = NUM_PAGES;
    }
    int* memory = (int*) calloc(memorySize, sizeof(int));
    if (memory == NULL) {
        fprintf(stderr, "Failed to allocate memory block\n");
        exit(EXIT_FAILURE);
    }
    return memory;
}

/*******************************************************************************************************/

/* Round-Robin Scheduling with Infinite Memory */
void infiniteRR(ProcessQueue* processQ, Process* processes, int processCount, int quantum) {
    int time, finished, remaining;
    time = finished = remaining = 0;

    while (finished < processCount) {

        /* check if any new processes need to added to the queue */
        checkProcesses(processQ, processes, processCount, time, &remaining,quantum);

        /* take the process at the head of the queue,
            this process is now considered in the CPU */
        Process* CPUproc;
        if (processQ->head != NULL) {
            CPUproc = (processQ->head->process);
        } 
        /* if there are no READY processes in the queue,
            the CPU will run idle for another quantum */
        else {
            time += quantum;
            continue;
        }

        /* this is the first instruction call. It will determine what action to take for the quantum.
            the instruction is either CONTINUE, FINISHED, or SWITCH*/
        int isNew = FALSE;
        int instruction = update(CPUproc, quantum, &time, &finished, remaining, &isNew);

        /* if the process was NEW in the CPU, print out a message */
        if (isNew) {
            int  remainingTime = CPUproc->serviceTime - CPUproc->cpuTimeUsed;
            printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", 
                    /* decrement time in message in accordance with qunatum */
                    (time - quantum), CPUproc->processName, (remainingTime + quantum));
        }

        /* a CONTINUE call indicates that update() allowed the current CPUproc to run for a quantum.
            if not, there is a call to switch out the current CPUproc */
        while (instruction != CONTINUE) {
            Process* sendBack = dequeue(processQ);

            /* if process is FINISHED, its serviceTime has been completed
                and it can be removed from the queue */
            if (sendBack->state == FINISHED) {
                remaining--;
                printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", time, sendBack->processName, remaining);
                sendBack->completionTime = time;
            
                /* continue as idlle if no other processes are queued*/
                if (remaining == 0) {
                    break;
                }

            /* if instructions is SWITCH, 
                which means that the current process has already run the last quantum, 
                AND there are also other processes READY,
                the CPU will switch out the process and send the current one to the back. */
            } else if (instruction == SWITCH) {
                sendBack->state = READY;
                enqueue(processQ, sendBack);
            }

            /* This update call will only be done IF a new process is now in the CPU */
            CPUproc = processQ->head->process;
            instruction = update(CPUproc, quantum, &time, &finished, remaining, &isNew);

            /* if the process was NEW in the CPU, print out a message */
            if (isNew) {
                int  remainingTime = CPUproc->serviceTime - CPUproc->cpuTimeUsed;
                printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", 
                    /* decrement time in message in accordance with qunatum */
                    (time - quantum), CPUproc->processName, (remainingTime + quantum));
            }
        } 
    }
}

/*******************************************************************************************************/

/* standard update function. Runs the CPU process for ONE time unit
*/
int update(Process* CPUproc, int quantum, int* time, int* finished, int remaining, int* isNew) {
    /* check whether the current CPU process has just entered, 
        or if it was already running */
    if (CPUproc->state == READY) {
        CPUproc->state = RUNNING;
        /* mark the process as NEW as up to this point, 
            the current CPU process was only considered a candidate */
        *isNew = TRUE;
    }

    /* check if quantum has elapsed for RUNNING process, 
        since process switching and completion can ONLY be performed at the end of a quantum.
        isNew indiciates whether a process has already run for a quantum or not */
    if ((CPUproc->state == RUNNING) && (!(*isNew))) {
        /* if at the end of a quantum, a processes CPUtime has passed its serviceTime,
            it is FINISHED */
        if (CPUproc->cpuTimeUsed >= CPUproc->serviceTime) {
            CPUproc->state = FINISHED;
            (*finished)++;
            return FINISHED;
        } 
        /* if a process has already run for a quantum and there are other READY processes in the queue,
            the process must be SWITCHed out */
        else if (remaining > 1) { 
            return SWITCH;
        } 
    }

    /* run the process for ONE quantum. 
        this will run ONLY if the process isNew
        or if there are no other processes in the queue */
    if ((CPUproc->state == RUNNING)) {
        /* increment CPU time used and totalTime by quantum */
        CPUproc->cpuTimeUsed += quantum;
        (*time) += quantum;  
    }

    return CONTINUE;
} 

/* Check for READY processes according to arrival time
*/
void checkProcesses(ProcessQueue* processQ, Process* processes, int processCount, int time, int* remaining, int quantum) {

    /* check if any processes are ready to enque based on arrival time */
    for (int i = 0; i < processCount; i++) {
        /* in the case that arrival time is not a multiple of the quantum,
            test if current time has overtaken arrival time,
            but only enque if arrival time was in between the current quantum and the previous */
        if ((processes[i].arrivalTime <= time) && (processes[i].arrivalTime > (time-quantum))) {
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
    int arrival, serviceTime, memoryReq;
    char* name = malloc(sizeof(char) * MAX_PROCESS_NAME_LEN);
    while (fscanf(fp, "%d %s %d %d", &arrival, name, &serviceTime, &memoryReq) == 4) {
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
        processes[*processCount].memoryRequirement = memoryReq;
        processes[*processCount].state = READY;
        processes[*processCount].cpuTimeUsed = READY;
        processes[*processCount].FFmemoryAllocation = NOT_ALLOCATED;
        processes[*processCount].PmemoryAllocation = NULL;
        processes[*processCount].sizeOfFrames = 0;
        processes[*processCount].lastUsed = NOT_ALLOCATED;
        processes[*processCount].completionTime = 0;
        (*processCount)++;
    }

    free(name);

    fclose(fp);
    return processes;
}

/* Process Queue Functions 

    Based on basic queue implentation using linked lists on
    https://www.geeksforgeeks.org/queue-linked-list-implementation/
*/
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