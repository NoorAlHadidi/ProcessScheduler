#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct
{
    Process *arr;
    int size;
} RRQueue;

RRQueue *createRRQueue()
{
    RRQueue *array = (RRQueue *)malloc(sizeof(RRQueue));
    array->size = 0;
    array->arr = NULL;
    return array;
}

void pushRR(RRQueue *array, Process data)
{
    array->size++;
    array->arr = (Process *)realloc(array->arr, array->size * sizeof(Process));
    array->arr[array->size - 1] = data;
}

void removeAtIndex(RRQueue *array, int index)
{
    if (index < 0 || index >= array->size)
    {
        printf("Invalid Index.\n");
        return;
    }
    for (int i = index; i < array->size - 1; ++i)
    {
        array->arr[i] = array->arr[i + 1];
    }
    array->size--;
    array->arr = (Process *)realloc(array->arr, array->size * sizeof(Process));
}

Process getElementAtIndex(RRQueue *array, int index)
{
    if (index < 0 || index >= array->size)
    {
        printf("Invalid Index.\n");
    }
    return array->arr[index];
}