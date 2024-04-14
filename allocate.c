#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************************************/
// List of Constants
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
const char *INFINITE_MEMORY = "infinite";

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
} Process;

/* Process Node
    to store the processes in a linked list 
*/
typedef struct ProcessNode{
    Process process;
    struct ProcessNode* next;
} ProcessNode;

typedef struct ProcessQueue {
    struct ProcessNode *head, *tail;
} ProcessQueue;

/*******************************************************************************************************/
// List of Functions

/* Queue Functions */
ProcessNode* newProcessNode(Process process);
ProcessQueue* createQueue();
void enqueue(ProcessQueue* processQueue, Process process);
Process dequeue(ProcessQueue* processQueue);

void allocate(Process* processes, int processCount, int quantum, char memoryStrategy[]);
void readInput(int argc, char* argv[], char filename[], char memoryStrategy[], int* quantum);
Process* readProcesses(char filename[], int* processCount);

void infinite_round_robin(Process* process_queue, int processcount, int quantum);
void printing_proc_output(int time, int state, char *name, int rtime, int proc_available);

/*******************************************************************************************************/
int main(int argc, char* argv[]) {
    char filename[MAX_FILENAME_STRATEGY_LEN], memoryStrategy[MAX_FILENAME_STRATEGY_LEN];
    int quantum;

    /* read command line arguments for simulation specifications */
    readInput(argc, argv, filename, memoryStrategy, &quantum);
    printf("filename: %s, memory strategy: %s, quantum: %d\n", filename, memoryStrategy, quantum);

    /* read and store processes to be simulated */
    int processCount;
    Process* processes = readProcesses(filename, &processCount);
    for (int i = 0; i < processCount; i++) {
        printf("Process Name: %s, Arrival: %d, ServiceT: %d, RemainingT: %d, Memory: %d, State: %d, CPU: %d\n",
                processes[i].processName, processes[i].arrivalTime, processes[i].serviceTime, processes[i].remainingTime,
                processes[i].memoryRequirement, processes[i].state, processes[i].cpuTimeUsed);
    }

    // Round robin with Infinite Memory
    if(strcmp(INFINITE_MEMORY, memoryStrategy)== 0){
        infinite_round_robin(processes, processCount, quantum);
    }
    //Notes: Make the original queue add processes
    // in order off arrival time

    return 0;
}

/* process manager:
    allocate processes into the CPU until all completed
*/
void allocate(Process* processes, int processCount, int quantum, char memoryStrategy[]) {
    ProcessQueue* CPU = createQueue();
}

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
        (*processCount)++;
    }

    fclose(fp);
    return processes;
}

// Process Queue Functions
ProcessNode* newProcessNode(Process process) {
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

void enqueue(ProcessQueue* processQueue, Process process) {
    ProcessNode* temp = newProcessNode(process);
    if (processQueue->tail = NULL) {
        processQueue->head = processQueue->tail = temp;
        return;
    }
    processQueue->tail->next = temp;
    processQueue->tail = temp;
}

Process dequeue(ProcessQueue* processQueue) {
    if (processQueue->head == NULL) {
        printf("Queue Empty\n");
        exit(EXIT_FAILURE);
    }
    ProcessNode* temp = processQueue->head;
    Process process = temp->process;
    processQueue->head = processQueue->head->next;
    if (processQueue->head == NULL) {
        processQueue->tail = NULL;
    }
    free(temp);
    return process;
}

void infinite_round_robin(Process* process_queue, int processcount, int quantum){

/*DELETE: NOTES FOR OURSELVES. Assume queue is in order of arrival time.
*/

    int curr_time;
    int total_finished_processes = 0;
    int current_proc_available = 0;
    int using_CPU = 0; // Checks whether a processor is currently using CPU

// NOTE: This loop assumes processes finishes only at the end of the quantum which is in the specs
    for(curr_time = 0; total_finished_processes <= processcount;){
        for(int i=0; i<processcount; i++){

            // Process runs for the first time.
            if(process_queue[i].arrivalTime == curr_time || (process_queue[i].arrivalTime >= curr_time && process_queue[i].arrivalTime < (curr_time-quantum)) ){
                
                process_queue[i].state = RUNNING;
                current_proc_available++;
                printing_proc_output(curr_time,process_queue[i].state,process_queue[i].processName,process_queue[i].remainingTime,current_proc_available); 
        
                curr_time += quantum;
                process_queue[i].remainingTime -= quantum;
            }
            
            else if(process_queue[i].arrivalTime < curr_time && process_queue[i].state == RUNNING){
                    curr_time += quantum;
                    process_queue[i].remainingTime -= quantum;
                    printing_proc_output(curr_time,process_queue[i].state,process_queue[i].processName,process_queue[i].remainingTime,current_proc_available); 
            }
            if(process_queue[i].remainingTime <= 0 && process_queue[i].state != FINISHED){
                process_queue[i].state = FINISHED;
                current_proc_available--;
                total_finished_processes++; 
                printing_proc_output(curr_time,process_queue[i].state,process_queue[i].processName,process_queue[i].remainingTime,current_proc_available);
                
            }
            
        }
        
    }
    //Note: Once a processor is finished, decrement total_ready_processes
        
} 

   void printing_proc_output(int time, int state, char *name, int rtime, int proc_available){
        if(state == 2){
            printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", time, name, proc_available);
        } else{
            printf("%d,RUNNING,process-name=%s,remaining-time=%d\n", time, name, rtime);
        }
   }