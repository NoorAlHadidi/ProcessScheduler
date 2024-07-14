#include "memory_struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_QUEUE_SIZE 200

typedef struct
{
    Process data[MAX_QUEUE_SIZE];
    int size;
} WaitingQueue;

void swapWait(Process *a, Process *b)
{
    Process temp = *a;
    *a = *b;
    *b = temp;
}

WaitingQueue *createWaitingQueue()
{
    WaitingQueue *wq = (WaitingQueue *)malloc(sizeof(WaitingQueue));
    wq->size = 0;
    return wq;
}

void heapifyUpWait(WaitingQueue *wq, int index)
{
    int parent = (index - 1) / 2;
    if (index > 0 && (wq->data[index].memSize < wq->data[parent].memSize || (wq->data[index].memSize == wq->data[parent].memSize && wq->data[index].id < wq->data[parent].id)))
    {
        swapWait(&wq->data[index], &wq->data[parent]);
        heapifyUpWait(wq, parent);
    }
}

void heapifyDownWait(WaitingQueue *wq, int index)
{
    int leftChild = 2 * index + 1;
    int rightChild = 2 * index + 2;
    int smallest = index;

    if (leftChild < wq->size && (wq->data[leftChild].memSize < wq->data[smallest].memSize || (wq->data[leftChild].memSize == wq->data[smallest].memSize && wq->data[leftChild].id < wq->data[smallest].id)))
        smallest = leftChild;
    if (rightChild < wq->size && (wq->data[rightChild].memSize < wq->data[smallest].memSize || (wq->data[rightChild].memSize == wq->data[smallest].memSize && wq->data[rightChild].id < wq->data[smallest].id)))
        smallest = rightChild;

    if (smallest != index)
    {
        swapWait(&wq->data[index], &wq->data[smallest]);
        heapifyDownWait(wq, smallest);
    }
}

void pushWait(WaitingQueue *wq, Process value)
{
    if (wq->size >= MAX_QUEUE_SIZE)
        return;
    wq->data[wq->size++] = value;
    heapifyUpWait(wq, wq->size - 1);
}

Process popWait(WaitingQueue *wq)
{
    Process min;
    if (wq->size <= 0)
        return min;
    min = wq->data[0];
    wq->data[0] = wq->data[wq->size - 1];
    wq->size--;
    heapifyDownWait(wq, 0);
    return min;
}

Process peekWait(WaitingQueue *wq)
{
    Process min;
    if (wq->size <= 0)
        return min;
    min = wq->data[0];
    return min;
}