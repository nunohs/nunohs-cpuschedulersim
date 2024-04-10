#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// List of Constants
/* All process-names will be distinct uppercase alphanumeric strings 
    of minimum length 1 and maximum length 8. (+1 for space)*/
#define MAX_PROCESS_NAME_LEN 9; 

/* Initial size of dynamic array storing processes */
#define INITIAL_PROCESSES 2;

/* Defined states of a process */
#define READY 0;
#define RUNNING 1;
#define FINISHED 2;

// List of Functions
void readInput(int argc, char* argv[], char** filename, char** memoryStrategy, int* quantum);
Process* readProcesses(const char* filename, int* processCount);

int main(int argc, char* argv[]) {

    int currentState = READY;
    return 0;
}

/* Process Structure Definition 
*/
typedef struct {
    int time_arrived;
    char processName[MAX_PROCESS_NAME_LEN];
    int serviceTime;
    int memoryRequirement;
    int state;
    int cpuTimeUsed;
} Process;

/* read command line arguments to deterimine:
    list of processes, memory strategy, & quantum length
*/
void readInput(int argc, char* argv[], char** filename, char** memoryStrategy, int* quantum){
    for (int i=1; i <= argc; i++) {
        /* filename */
        if (strcmp(argv[i], "-f") == 0) {
            filename = argv[++i];
        /* memory strategy (infinite, first-fit, paged, virtual)*/
        } else if (strcmp(argv[i], "-m") == 0) {
            memoryStrategy = argv[++i];
        /* quantum length (1, 2, 3)*/
        } else if (strcmp(argv[i], "-q") == 0) {
            quantum = argv[++i];
        }
    }
}

/* read list of processes from file and store their info
*/
Process* readProcesses(const char* filename, int* processCount) {
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
    char name;
    while (fscanf(fp, %d %s %d %d\n, &arrival, &name, &serviceTime, &memoryReq))
}