#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_QUEUE_SIZE 200

typedef struct
{
    Process data[MAX_QUEUE_SIZE];
    int size;
} SRTNQueue;

void swapSRTN(Process *a, Process *b)
{
    Process temp = *a;
    *a = *b;
    *b = temp;
}

SRTNQueue *createSRTNQueue()
{
    SRTNQueue *pq = (SRTNQueue *)malloc(sizeof(SRTNQueue));
    pq->size = 0;
    return pq;
}

void heapifyUpSRTN(SRTNQueue *pq, int index)
{
    int parent = (index - 1) / 2;
    if (index > 0 && (pq->data[index].remainingTime < pq->data[parent].remainingTime || (pq->data[index].remainingTime == pq->data[parent].remainingTime && pq->data[index].id < pq->data[parent].id)))
    {
        swapSRTN(&pq->data[index], &pq->data[parent]);
        heapifyUpSRTN(pq, parent);
    }
}

void heapifyDownSRTN(SRTNQueue *pq, int index)
{
    int leftChild = 2 * index + 1;
    int rightChild = 2 * index + 2;
    int smallest = index;

    if (leftChild < pq->size && (pq->data[leftChild].remainingTime < pq->data[smallest].remainingTime || (pq->data[leftChild].remainingTime == pq->data[smallest].remainingTime && pq->data[leftChild].id < pq->data[smallest].id)))
        smallest = leftChild;
    if (rightChild < pq->size && (pq->data[rightChild].remainingTime < pq->data[smallest].remainingTime || (pq->data[rightChild].remainingTime == pq->data[smallest].remainingTime && pq->data[rightChild].id < pq->data[smallest].id)))
        smallest = rightChild;

    if (smallest != index)
    {
        swapSRTN(&pq->data[index], &pq->data[smallest]);
        heapifyDownSRTN(pq, smallest);
    }
}

void pushSRTN(SRTNQueue *pq, Process value)
{
    if (pq->size >= MAX_QUEUE_SIZE)
        return;
    pq->data[pq->size++] = value;
    heapifyUpSRTN(pq, pq->size - 1);
}

Process popSRTN(SRTNQueue *pq)
{
    Process min;
    if (pq->size <= 0)
        return min;
    min = pq->data[0];
    pq->data[0] = pq->data[pq->size - 1];
    pq->size--;
    heapifyDownSRTN(pq, 0);
    return min;
}
