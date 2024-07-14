#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct
{
    int id;
    int arrivalTime;
    int runTime;
    int priority;
    int waitingTime;
    int remainingTime;
    int startTime;
    int finishTime;
    int memSize;
    int memStartIndex;
    int memEndIndex;
    bool isRunning;
} Process;

void initProcess(Process *process, int id, int arrivalTime, int runTime, int priority, int memSize)
{
    process->id = id;
    process->arrivalTime = arrivalTime;
    process->runTime = runTime;
    process->priority = priority;
    process->remainingTime = runTime;
    process->waitingTime = 0;
    process->startTime = -1;
    process->finishTime = -1;
    process->memSize = memSize;
    process->memStartIndex = -1;
    process->memEndIndex = -1;
    process->isRunning = false; // initially ready
}

typedef struct
{
    int size;
    Process **elements;
} ProcessList;

ProcessList *createList()
{
    ProcessList *list = (ProcessList *)malloc(sizeof(ProcessList));
    list->elements = NULL;
    list->size = 0;
    return list;
}

void addProcess(ProcessList *list, Process *element)
{
    list->size++;
    list->elements = (Process **)realloc(list->elements, list->size * sizeof(Process *));
    list->elements[list->size - 1] = element;
}

struct msgbuff
{
    long mtype;
    Process mprocess;
};

struct runbuff
{
    long mtype;
    bool misrunning;
};

