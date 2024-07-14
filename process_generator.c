#include "headers.h"
#include "process_struct.h"

void clearResources(int signum);

int pQueueID;
int main(int argc, char *argv[])
{
    if (signal(SIGINT, clearResources) == SIG_ERR)
    {
        printf("Error in attaching to the signal handler.\n");
        fflush(stdout);
    }

    // TODO initialization
    int currProc = 0;
    key_t pQueue = ftok("keyfile", 'P');
    pQueueID = msgget(pQueue, 0666 | IPC_CREAT); // message queue between process_generator and scheduler
    struct msgbuff pMessage;
    int sendValue;

    // 1. read the input files.
    FILE *pFile;
    pFile = fopen("processes.txt", "r");
    if (pFile == NULL)
    {
        printf("Error in reading the input file.\n");
    }
    char line[100];
    fgets(line, sizeof(line), pFile); // to skip first line in input file
    int info[5];
    ProcessList *pList = createList();
    while ((fscanf(pFile, "%d", &info[0])) == 1)
    {
        fscanf(pFile, "%d", &info[1]); // arrival time
        fscanf(pFile, "%d", &info[2]); // run time
        fscanf(pFile, "%d", &info[3]); // priority
        fscanf(pFile, "%d", &info[4]); // memory size
        Process *process = malloc(sizeof(Process));
        initProcess(process, info[0], info[1], info[2], info[3], info[4]);
        addProcess(pList, process);
    }
    // 2. ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    char schedAlgo[10];
    char quantum[10];
    char processCount[20];
    snprintf(processCount, sizeof(processCount), "%d", pList->size);
    printf("Enter 1 for HPF, 2 for SRTN, 3 for RR: \n");
    scanf("%s", schedAlgo);
    if (atoi(schedAlgo) == 3) // RR takes quantum as an extra parameter
    {
        printf("Enter the quantum: \n");
        scanf("%s", quantum);
    }

    // 3. initiate and create the scheduler and clock processes.

    int schedulerPID = fork();
    if (schedulerPID == 0)
    {
        execl("./scheduler.out", "./scheduler.out", schedAlgo, quantum, processCount, NULL);
    }
    sleep(5); // ensure that clock doesn't precede scheduler (scheduler does initializations before clock is forked)
    int clockPID = fork();
    if (clockPID == 0)
    {
        execl("./clk.out", "./clk.out", NULL);
    }
    // 4. use this function after creating the clock process to initialize clock
    initClk(); // attaching to clock shared memory

    // TODO generation main loop
    // 5. create a data structure for processes and provide it with its parameters.
    // 6. send the information to the scheduler at the appropriate time.
    int counter = -1; // to start counting when clock = 0
    while (true)
    {
        if (getClk() == counter + 1) // make sure 1 second passed
        {
            while (currProc != pList->size && getClk() == (pList->elements[currProc])->arrivalTime)
            {
                pMessage.mprocess = *pList->elements[currProc];
                pMessage.mtype = 1;
                sendValue = msgsnd(pQueueID, &pMessage, sizeof(pMessage.mprocess), !IPC_NOWAIT); // send process to scheduler
                currProc++;
            }
            pMessage.mtype = 5; // channel for sending dummy process when no process arrives at that time
            sendValue = msgsnd(pQueueID, &pMessage, sizeof(pMessage.mprocess), !IPC_NOWAIT);
            counter++;
        }
    }
    // 7. clear clock resources
    destroyClk(false);
    printf("Detaching from clock.\n");

    return 0;
}

void clearResources(int signum)
{
    // TODO clears all resources in case of interruption
    printf("Clearing IPC resources.\n");
    fflush(stdout);
    destroyClk(false);
    msgctl(pQueueID, IPC_RMID, (struct msqid_ds *)0);
    exit(signum);
}