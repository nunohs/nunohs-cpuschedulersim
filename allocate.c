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

/*******************************************************************************************************/
/* Process Structure Definition 
*/
typedef struct Process {
    int arrivalTime;
    char processName[MAX_PROCESS_NAME_LEN];
    int serviceTime;
    int memoryRequirement;
    int state;
    int cpuTimeUsed;
} Process;

/*******************************************************************************************************/
// List of Functions
void readInput(int argc, char* argv[], char filename[], char memoryStrategy[], int* quantum);
Process* readProcesses(char filename[], int* processCount);

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
        printf("Process Name: %s, Arrival: %d, ServiceT: %d, Memory: %d, State: %d, CPU: %d\n",
                processes[i].processName, processes[i].arrivalTime, processes[i].serviceTime,
                processes[i].memoryRequirement, processes[i].state, processes[i].cpuTimeUsed);
    }

    return 0;
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
    int arrival, serviceTime, memoryReq;
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
        processes[*processCount].memoryRequirement = memoryReq;
        processes[*processCount].state = READY;
        processes[*processCount].cpuTimeUsed = READY;
        (*processCount)++;
    }

    fclose(fp);
    return processes;
}