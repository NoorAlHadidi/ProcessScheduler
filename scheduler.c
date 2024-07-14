#include "headers.h"
#include "waiting_queue.h"
#include "hpf_queue.h"
#include "srtn_queue.h"
#include "rr_queue.h"
#include <math.h>

int pQueueID, runQueueID, detailsQueueID;
struct msgbuff pMessage;
Process *runningProcess = NULL;
int schedAlgo, quantum, processCount, finishedProcessCount;
Process *processTable = NULL;
float busy, time, counter;

void start(Process process);
void stop(Process process);

int askForDetails()
{
    struct msgbuff tempMessage;
    int recValue = msgrcv(detailsQueueID, &tempMessage, sizeof(tempMessage.mprocess), 0, !IPC_NOWAIT);
    if (tempMessage.mprocess.id == -1)
    {
        return -1;
    }
    if (recValue != -1)
    {
        processTable[tempMessage.mprocess.id].waitingTime = tempMessage.mprocess.waitingTime;
        processTable[tempMessage.mprocess.id].remainingTime = tempMessage.mprocess.remainingTime;
        processTable[tempMessage.mprocess.id].startTime = tempMessage.mprocess.startTime;
        processTable[tempMessage.mprocess.id].finishTime = tempMessage.mprocess.finishTime;
        runningProcess = NULL;
        return tempMessage.mprocess.id;
    }
    return -1;
}

int askForSpecificDetails(int id)
{
    struct msgbuff tempMessage;
    int recValue = msgrcv(detailsQueueID, &tempMessage, sizeof(tempMessage.mprocess), id, !IPC_NOWAIT);
    if (recValue != -1)
    {
        processTable[tempMessage.mprocess.id].waitingTime = tempMessage.mprocess.waitingTime;
        processTable[tempMessage.mprocess.id].remainingTime = tempMessage.mprocess.remainingTime;
        processTable[tempMessage.mprocess.id].startTime = tempMessage.mprocess.startTime;
        processTable[tempMessage.mprocess.id].finishTime = tempMessage.mprocess.finishTime;
        return tempMessage.mprocess.id;
    }
    return -1;
}

void calculatePerformance(FILE *perf)
{
    float utilization = 0;
    float avgWT = 0;
    float avgWTA = 0;
    float stdWTA = 0;

    for (int i = 1; i < processCount + 1; i++)
    {
        avgWT += (float)processTable[i].waitingTime;
        if (processTable[i].runTime == 0)
            continue;
        avgWTA += ((float)(processTable[i].finishTime - processTable[i].arrivalTime) / (float)processTable[i].runTime);
    }
    avgWT /= (float)processCount;
    avgWTA /= (float)processCount;
    for (int i = 1; i < processCount + 1; i++)
    {
        if (processTable[i].runTime == 0)
            continue;
        float diff = ((float)(processTable[i].finishTime - processTable[i].arrivalTime) / (float)processTable[i].runTime) - avgWTA;
        stdWTA += diff * diff;
    }
    stdWTA /= (float)processCount;
    stdWTA = sqrt(stdWTA);
    utilization = 100 * busy / counter;

    fprintf(perf, "CPU utilization = %.2f%% \n", utilization);
    fflush(perf);
    fprintf(perf, "Avg WTA = %.2f \n", avgWTA);
    fflush(perf);
    fprintf(perf, "Avg Waiting = %.2f \n", avgWT);
    fflush(perf);
    fprintf(perf, "Std WTA = %.2f \n", stdWTA);
    fflush(perf);
}

int main(int argc, char *argv[])
{
    finishedProcessCount = 0;

    schedAlgo = atoi(argv[1]);
    quantum = atoi(argv[2]);
    processCount = atoi(argv[3]);
    processTable = (Process *)malloc((processCount + 1) * sizeof(Process)); // one-based index for PCB

    FILE *log = fopen("scheduler.log", "w");
    FILE *perf = fopen("scheduler.perf", "w");
    FILE *mem = fopen("memory.log", "w");
    fprintf(log, "#At time x process y state arr w total z remaining y wait k\n");
    fflush(log);
    fprintf(mem, "#At time x allocated y bytes for process z from i to j\n");
    fflush(mem);

    key_t pQueue = ftok("keyfile", 'P');
    pQueueID = msgget(pQueue, 0666 | IPC_CREAT);

    key_t runQueue = ftok("keyfile", 'R');
    runQueueID = msgget(runQueue, 0666 | IPC_CREAT);

    key_t detailsQueue = ftok("keyfile", 'D');
    detailsQueueID = msgget(detailsQueue, 0666 | IPC_CREAT);

    Memory *memory = createMemory();
    WaitingQueue *waitQueue = createWaitingQueue();

    initClk();
    printf("Scheduler starting at time %d.\n", getClk());
    fflush(stdout);

    busy = 0;
    time = 0;
    counter = getClk();

    if (schedAlgo == 1) // HPF
    {
        PriorityQueue *hpfQueue = createPriorityQueue();
        while (true)
        {
            while (msgrcv(pQueueID, &pMessage, sizeof(pMessage.mprocess), 0, !IPC_NOWAIT) != -1)
            {
                if (pMessage.mtype == 5) // dummy process
                {
                    break;
                }
                if (allocateMemory(&pMessage.mprocess, memory)) // allocate memory block to process
                {
                    int processPID = fork();
                    if (processPID == 0)
                    {
                        char idString[20];
                        char remainingTimeString[20];
                        char arrivalTimeString[20];
                        snprintf(idString, sizeof(idString), "%d", pMessage.mprocess.id);
                        snprintf(remainingTimeString, sizeof(remainingTimeString), "%d", pMessage.mprocess.remainingTime);
                        snprintf(arrivalTimeString, sizeof(arrivalTimeString), "%d", pMessage.mprocess.arrivalTime);
                        execl("./process.out", "./process.out", idString, remainingTimeString, arrivalTimeString, NULL);
                    }
                    fprintf(mem, "At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), (pMessage.mprocess.memEndIndex - pMessage.mprocess.memStartIndex + 1), pMessage.mprocess.id, pMessage.mprocess.memStartIndex, pMessage.mprocess.memEndIndex);
                    fflush(mem);
                    pushHPF(hpfQueue, pMessage.mprocess);
                    processTable[pMessage.mprocess.id] = pMessage.mprocess; // indexing PCB with process ID
                }
                else
                {
                    pushWait(waitQueue, pMessage.mprocess); // cannot allocate memory, process waits
                }
            }
            // tries to receive terminating process details
            if (runningProcess != NULL)
            {
                int id = askForDetails();
                printf("At time %d, process ID is %d.\n", getClk(), id);
                fflush(stdout);
                if (id != -1) // no process terminated
                {
                    int at = processTable[id].arrivalTime;
                    int ct = processTable[id].runTime;
                    int rt = processTable[id].remainingTime;
                    int wt = processTable[id].waitingTime;
                    int ta = (processTable[id].finishTime - at);
                    float wta = (float)ta / (float)ct;
                    fprintf(log, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f \n", getClk(), id, at, ct, rt, wt, ta, wta);
                    fflush(log);
                    deallocateMemory(&processTable[id], memory);
                    fprintf(mem, "At time %d freed %d bytes from process %d from %d to %d\n", getClk(), (processTable[id].memEndIndex - processTable[id].memStartIndex + 1), id, processTable[id].memStartIndex, processTable[id].memEndIndex);
                    fflush(mem);
                    finishedProcessCount++;
                    while (waitQueue->size > 0) // allocate memory to eligible waiting processes
                    {
                        Process tempWait = peekWait(waitQueue);
                        if (allocateMemory(&tempWait, memory))
                        {
                            popWait(waitQueue);
                            pushHPF(hpfQueue, tempWait);
                            processTable[tempWait.id] = tempWait; // indexing PCB with process ID
                            int processPID = fork();
                            if (processPID == 0)
                            {
                                char idString[20];
                                char remainingTimeString[20];
                                char arrivalTimeString[20];
                                snprintf(idString, sizeof(idString), "%d", tempWait.id);
                                snprintf(remainingTimeString, sizeof(remainingTimeString), "%d", tempWait.remainingTime);
                                snprintf(arrivalTimeString, sizeof(arrivalTimeString), "%d", tempWait.arrivalTime);
                                execl("./process.out", "./process.out", idString, remainingTimeString, arrivalTimeString, NULL);
                            }
                            fprintf(mem, "At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), (tempWait.memEndIndex - tempWait.memStartIndex + 1), tempWait.id, tempWait.memStartIndex, tempWait.memEndIndex);
                            fflush(mem);
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            if (runningProcess == NULL && hpfQueue->size > 0)
            {
                Process tempProcess = popHPF(hpfQueue);
                runningProcess = &tempProcess;
                start(tempProcess);
                // output started process state
                int id = askForSpecificDetails(tempProcess.id);
                int at = processTable[id].arrivalTime;
                int ct = processTable[id].runTime;
                int rt = processTable[id].remainingTime;
                int wt = processTable[id].waitingTime;
                if (processTable[id].startTime == -1)
                    fprintf(log, "At time %d process %d started arr %d total %d remain %d wait %d \n", getClk(), id, at, ct, rt, wt);
                else
                    fprintf(log, "At time %d process %d resumed arr %d total %d remain %d wait %d \n", getClk(), id, at, ct, rt, wt);
                fflush(log);
            }
            if (getClk() == counter + 1)
            {
                counter++;
                if (runningProcess != NULL)
                    busy++;
                time++;
            }
            if (finishedProcessCount == processCount)
                break;
        }
    }
    if (schedAlgo == 2) // SRTN
    {
        SRTNQueue *srtnQueue = createSRTNQueue();
        while (true)
        {
            int newArrivals = 0;
            while (msgrcv(pQueueID, &pMessage, sizeof(pMessage.mprocess), 0, !IPC_NOWAIT) != -1)
            {
                if (pMessage.mtype == 5) // dummy process
                {
                    break;
                }
                if (allocateMemory(&pMessage.mprocess, memory)) // allocate memory block to process
                {
                    newArrivals = 1;
                    int processPID = fork();
                    if (processPID == 0)
                    {
                        char idString[20];
                        char remainingTimeString[20];
                        char arrivalTimeString[20];
                        snprintf(idString, sizeof(idString), "%d", pMessage.mprocess.id);
                        snprintf(remainingTimeString, sizeof(remainingTimeString), "%d", pMessage.mprocess.remainingTime);
                        snprintf(arrivalTimeString, sizeof(arrivalTimeString), "%d", pMessage.mprocess.arrivalTime);
                        execl("./process.out", "./process.out", idString, remainingTimeString, arrivalTimeString, NULL);
                    }
                    fprintf(mem, "At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), (pMessage.mprocess.memEndIndex - pMessage.mprocess.memStartIndex + 1), pMessage.mprocess.id, pMessage.mprocess.memStartIndex, pMessage.mprocess.memEndIndex);
                    fflush(mem);
                    pushSRTN(srtnQueue, pMessage.mprocess);
                    processTable[pMessage.mprocess.id] = pMessage.mprocess;
                }
                else
                {
                    pushWait(waitQueue, pMessage.mprocess); // cannot allocate memory, process waits
                }
            }
            // tries to receive terminating process details
            if (runningProcess != NULL)
            {
                int id = askForDetails();
                printf("At time %d, process ID is %d.\n", getClk(), id);
                fflush(stdout);
                if (id != -1)
                {
                    int at = processTable[id].arrivalTime;
                    int ct = processTable[id].runTime;
                    int rt = processTable[id].remainingTime;
                    int wt = processTable[id].waitingTime;
                    int ta = (processTable[id].finishTime - at);
                    float wta = (float)ta / (float)ct;
                    fprintf(log, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f \n", getClk(), id, at, ct, rt, wt, ta, wta);
                    fflush(log);
                    deallocateMemory(&processTable[id], memory);
                    fprintf(mem, "At time %d freed %d bytes from process %d from %d to %d\n", getClk(), (processTable[id].memEndIndex - processTable[id].memStartIndex + 1), id, processTable[id].memStartIndex, processTable[id].memEndIndex);
                    fflush(mem);
                    finishedProcessCount++;
                    while (waitQueue->size > 0) // allocate memory to eligible waiting processes
                    {
                        Process tempWait = peekWait(waitQueue);
                        if (allocateMemory(&tempWait, memory))
                        {
                            popWait(waitQueue);
                            pushSRTN(srtnQueue, tempWait);
                            processTable[tempWait.id] = tempWait; // indexing PCB with process ID
                            int processPID = fork();
                            if (processPID == 0)
                            {
                                char idString[20];
                                char remainingTimeString[20];
                                char arrivalTimeString[20];
                                snprintf(idString, sizeof(idString), "%d", tempWait.id);
                                snprintf(remainingTimeString, sizeof(remainingTimeString), "%d", tempWait.remainingTime);
                                snprintf(arrivalTimeString, sizeof(arrivalTimeString), "%d", tempWait.arrivalTime);
                                execl("./process.out", "./process.out", idString, remainingTimeString, arrivalTimeString, NULL);
                            }
                            fprintf(mem, "At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), (tempWait.memEndIndex - tempWait.memStartIndex + 1), tempWait.id, tempWait.memStartIndex, tempWait.memEndIndex);
                            fflush(mem);
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            if (runningProcess != NULL && newArrivals == 1) // check for pre-emption; context-switch regardless of process
            {
                stop(*runningProcess);
                // output stopped process state
                int id = askForSpecificDetails(runningProcess->id);
                int at = processTable[id].arrivalTime;
                int ct = processTable[id].runTime;
                int rt = processTable[id].remainingTime;
                int wt = processTable[id].waitingTime;
                fprintf(log, "At time %d process %d stopped arr %d total %d remain %d wait %d \n", getClk(), id, at, ct, rt, wt);
                fflush(log);
                pushSRTN(srtnQueue, processTable[id]); // push into queue with updated details
                runningProcess = NULL;
            }
            if (runningProcess == NULL && srtnQueue->size > 0)
            {
                Process tempProcess = popSRTN(srtnQueue);
                runningProcess = &tempProcess;
                start(tempProcess);
                // output started process state
                int id = askForSpecificDetails(tempProcess.id);
                int at = processTable[id].arrivalTime;
                int ct = processTable[id].runTime;
                int rt = processTable[id].remainingTime;
                int wt = processTable[id].waitingTime;
                if (processTable[id].startTime == -1)
                    fprintf(log, "At time %d process %d started arr %d total %d remain %d wait %d \n", getClk(), id, at, ct, rt, wt);
                else
                    fprintf(log, "At time %d process %d resumed arr %d total %d remain %d wait %d \n", getClk(), id, at, ct, rt, wt);
                fflush(log);
            }
            if (getClk() == counter + 1)
            {
                counter++;
                if (runningProcess != NULL)
                    busy++;
                time++;
            }
            if (finishedProcessCount == processCount)
                break;
        }
    }
    if (schedAlgo == 3) // RR
    {
        int iterator = -1;
        int timeSlice = quantum;
        RRQueue *rrQueue = createRRQueue();
        while (true)
        {
            while (msgrcv(pQueueID, &pMessage, sizeof(pMessage.mprocess), 0, !IPC_NOWAIT) != -1)
            {
                if (pMessage.mtype == 5)
                {
                    break;
                }
                if (allocateMemory(&pMessage.mprocess, memory)) // allocate memory block to process
                {
                    int processPID = fork();
                    if (processPID == 0)
                    {
                        char idString[20];
                        char remainingTimeString[20];
                        char arrivalTimeString[20];
                        snprintf(idString, sizeof(idString), "%d", pMessage.mprocess.id);
                        snprintf(remainingTimeString, sizeof(remainingTimeString), "%d", pMessage.mprocess.remainingTime);
                        snprintf(arrivalTimeString, sizeof(arrivalTimeString), "%d", pMessage.mprocess.arrivalTime);
                        execl("./process.out", "./process.out", idString, remainingTimeString, arrivalTimeString, NULL);
                    }
                    fprintf(mem, "At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), (pMessage.mprocess.memEndIndex - pMessage.mprocess.memStartIndex + 1), pMessage.mprocess.id, pMessage.mprocess.memStartIndex, pMessage.mprocess.memEndIndex);
                    fflush(mem);
                    pushRR(rrQueue, pMessage.mprocess);
                    processTable[pMessage.mprocess.id] = pMessage.mprocess;
                }
                else
                {
                    pushWait(waitQueue, pMessage.mprocess); // cannot allocate memory, process waits
                }
            }
            if (runningProcess != NULL)
            {
                timeSlice--;
            }
            // tries to receive terminating process details
            if (runningProcess != NULL)
            {
                int id = askForDetails();
                printf("At time %d, process ID is %d.\n", getClk(), id);
                fflush(stdout);
                if (id != -1)
                {
                    int at = processTable[id].arrivalTime;
                    int ct = processTable[id].runTime;
                    int rt = processTable[id].remainingTime;
                    int wt = processTable[id].waitingTime;
                    int ta = (processTable[id].finishTime - at);
                    float wta = (float)ta / (float)ct;
                    fprintf(log, "At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %.2f \n", getClk(), id, at, ct, rt, wt, ta, wta);
                    fflush(log);
                    deallocateMemory(&processTable[id], memory);
                    fprintf(mem, "At time %d freed %d bytes from process %d from %d to %d\n", getClk(), (processTable[id].memEndIndex - processTable[id].memStartIndex + 1), id, processTable[id].memStartIndex, processTable[id].memEndIndex);
                    fflush(mem);
                    finishedProcessCount++;
                    removeAtIndex(rrQueue, iterator);
                    while (waitQueue->size > 0) // allocate memory to eligible waiting processes
                    {
                        Process tempWait = peekWait(waitQueue);
                        if (allocateMemory(&tempWait, memory))
                        {
                            popWait(waitQueue);
                            pushRR(rrQueue, tempWait);
                            processTable[tempWait.id] = tempWait; // indexing PCB with process ID
                            int processPID = fork();
                            if (processPID == 0)
                            {
                                char idString[20];
                                char remainingTimeString[20];
                                char arrivalTimeString[20];
                                snprintf(idString, sizeof(idString), "%d", tempWait.id);
                                snprintf(remainingTimeString, sizeof(remainingTimeString), "%d", tempWait.remainingTime);
                                snprintf(arrivalTimeString, sizeof(arrivalTimeString), "%d", tempWait.arrivalTime);
                                execl("./process.out", "./process.out", idString, remainingTimeString, arrivalTimeString, NULL);
                            }
                            fprintf(mem, "At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), (tempWait.memEndIndex - tempWait.memStartIndex + 1), tempWait.id, tempWait.memStartIndex, tempWait.memEndIndex);
                            fflush(mem);
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (iterator == rrQueue->size)
                        iterator = -1;
                    else
                        iterator--; // make sure iterator stays where it is incase of shifting
                }
            }
            if (runningProcess != NULL && timeSlice == 0) // pre-emption
            {
                stop(*runningProcess);
                printf("At time %d process %d stopped arr %d total %d remain %d wait %d \n", getClk(), runningProcess->id, runningProcess->arrivalTime, runningProcess->runTime, runningProcess->remainingTime, runningProcess->waitingTime);
                fflush(stdout);
                // output stopped process state
                int id = askForSpecificDetails(runningProcess->id);
                int at = processTable[id].arrivalTime;
                int ct = processTable[id].runTime;
                int rt = processTable[id].remainingTime;
                int wt = processTable[id].waitingTime;
                fprintf(log, "At time %d process %d stopped arr %d total %d remain %d wait %d \n", getClk(), id, at, ct, rt, wt);
                fflush(log);
                runningProcess = NULL;
            }
            if (runningProcess == NULL && rrQueue->size > 0)
            {
                timeSlice = quantum;
                iterator = (iterator + 1) % (rrQueue->size);
                Process tempProcess = getElementAtIndex(rrQueue, iterator);
                runningProcess = &tempProcess;
                start(tempProcess);
                // output started process state
                int id = askForSpecificDetails(tempProcess.id);
                int at = processTable[id].arrivalTime;
                int ct = processTable[id].runTime;
                int rt = processTable[id].remainingTime;
                int wt = processTable[id].waitingTime;
                if (processTable[id].startTime == -1)
                    fprintf(log, "At time %d process %d started arr %d total %d remain %d wait %d \n", getClk(), id, at, ct, rt, wt);
                else
                    fprintf(log, "At time %d process %d resumed arr %d total %d remain %d wait %d \n", getClk(), id, at, ct, rt, wt);
                fflush(log);
            }
            if (getClk() == counter + 1)
            {
                counter++;
                if (runningProcess != NULL)
                {
                    busy++;
                }
                time++;
            }
            if (finishedProcessCount == processCount)
                break;
        }
    }
    calculatePerformance(perf);
    printf("Clearing IPC resources.\n");
    msgctl(runQueueID, IPC_RMID, (struct msqid_ds *)0);
    msgctl(detailsQueueID, IPC_RMID, (struct msqid_ds *)0);
    fclose(log);
    fclose(perf);
    fclose(mem);
    destroyClk(true);
    return 0;
    // TODO implement the scheduler :)
    // upon termination release the clock resources
}

void start(Process process)
{
    struct runbuff rMessage;
    rMessage.misrunning = true;
    rMessage.mtype = process.id;
    int sendValue = msgsnd(runQueueID, &rMessage, sizeof(rMessage.misrunning), !IPC_NOWAIT);
}

void stop(Process process)
{
    struct runbuff rMessage;
    rMessage.misrunning = false;
    rMessage.mtype = process.id;
    int sendValue = msgsnd(runQueueID, &rMessage, sizeof(rMessage.misrunning), !IPC_NOWAIT);
}