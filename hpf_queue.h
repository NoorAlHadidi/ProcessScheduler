#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_QUEUE_SIZE 200

typedef struct
{
    Process data[MAX_QUEUE_SIZE];
    int size;
} PriorityQueue;

void swapHPF(Process *a, Process *b)
{
    Process temp = *a;
    *a = *b;
    *b = temp;
}

PriorityQueue *createPriorityQueue()
{
    PriorityQueue *pq = (PriorityQueue *)malloc(sizeof(PriorityQueue));
    pq->size = 0;
    return pq;
}

void heapifyUpHPF(PriorityQueue *pq, int index)
{
    int parent = (index - 1) / 2;
    if (index > 0 && (pq->data[index].priority < pq->data[parent].priority || (pq->data[index].priority == pq->data[parent].priority && pq->data[index].id < pq->data[parent].id)))
    {
        swapHPF(&pq->data[index], &pq->data[parent]);
        heapifyUpHPF(pq, parent);
    }
}

void heapifyDownHPF(PriorityQueue *pq, int index)
{
    int leftChild = 2 * index + 1;
    int rightChild = 2 * index + 2;
    int smallest = index;

    if (leftChild < pq->size && (pq->data[leftChild].priority < pq->data[smallest].priority || (pq->data[leftChild].priority == pq->data[smallest].priority && pq->data[leftChild].id < pq->data[smallest].id)))
        smallest = leftChild;
    if (rightChild < pq->size && (pq->data[rightChild].priority < pq->data[smallest].priority || (pq->data[rightChild].priority == pq->data[smallest].priority && pq->data[rightChild].id < pq->data[smallest].id)))
        smallest = rightChild;

    if (smallest != index)
    {
        swapHPF(&pq->data[index], &pq->data[smallest]);
        heapifyDownHPF(pq, smallest);
    }
}

void pushHPF(PriorityQueue *pq, Process value)
{
    if (pq->size >= MAX_QUEUE_SIZE)
        return;
    pq->data[pq->size++] = value;
    heapifyUpHPF(pq, pq->size - 1);
}

Process popHPF(PriorityQueue *pq)
{
    Process min;
    if (pq->size <= 0)
        return min;
    min = pq->data[0];
    pq->data[0] = pq->data[pq->size - 1];
    pq->size--;
    heapifyDownHPF(pq, 0);
    return min;
}
