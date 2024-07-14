#include "headers.h"
#include "process_struct.h"

/* modify this file as needed*/
int id, arrivalTime, remainingTime, waitingTime, startTime, finishTime, counter;
bool isRunning;

int runQueueID;     // message queue between scheduler and process
int detailsQueueID; // message queue between process and scheduler

void sendDetails(bool ignore)
{
    struct msgbuff pMessage;
    Process process;
    process.id = id;
    process.waitingTime = waitingTime;
    process.remainingTime = remainingTime;
    process.startTime = startTime;
    process.finishTime = finishTime;
    if (ignore) // no process started/resumed/stopped/terminated
        process.id = -1;
    pMessage.mtype = id;
    pMessage.mprocess = process;
    int sendValue = msgsnd(detailsQueueID, &pMessage, sizeof(pMessage.mprocess), !IPC_NOWAIT); // sends process details to scheduler
}

int main(int agrc, char *argv[])
{
    initClk();

    // TODO it needs to get the remaining time from somewhere
    id = atoi(argv[1]);
    remainingTime = atoi(argv[2]);
    arrivalTime = atoi(argv[3]);
    startTime = -1;
    finishTime = -1;
    isRunning = false;
    counter = getClk();
    waitingTime = getClk() - arrivalTime; 
    
    key_t runQueue = ftok("keyfile", 'R');
    runQueueID = msgget(runQueue, 0666 | IPC_CREAT);
    key_t detailsQueue = ftok("keyfile", 'D');
    detailsQueueID = msgget(detailsQueue, 0666 | IPC_CREAT);
    while (remainingTime > 0)
    {
        if (getClk() == counter + 1)
        {
            if (isRunning)
            {
                remainingTime--;
                if (remainingTime == 0)
                    break;
                if (startTime == -1) // process newly started not resumed
                    startTime = getClk();
                sendDetails(true);
            }
            else
            {
                waitingTime++;
            }
            counter++;
        }
        struct runbuff rMessage;
        int recValue = msgrcv(runQueueID, &rMessage, sizeof(rMessage.misrunning), id, IPC_NOWAIT); // process receives from scheduler whether it's running or not
        if (recValue != -1)
        {
            isRunning = rMessage.misrunning;
            sendDetails(false);
        }
    }

    finishTime = getClk();
    sendDetails(false);
    destroyClk(false);
    return 0;
}
