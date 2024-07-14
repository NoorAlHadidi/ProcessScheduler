#include "process_struct.h"

#define MAX_MEMORY_SIZE 1024

typedef struct
{
    int startIndex, endIndex, blockSize;
    bool isFree;
} MemBlock;

void initBlock(MemBlock *block, int start, int end)
{
    block->startIndex = start;
    block->endIndex = end;
    block->blockSize = (end - start + 1);
    block->isFree = true; // initially free
}

typedef struct
{
    MemBlock data[MAX_MEMORY_SIZE];
    int size;
} Memory;

void swapMemory(MemBlock *a, MemBlock *b)
{
    MemBlock temp = *a;
    *a = *b;
    *b = temp;
}

int shiftUpMemory(Memory *mem, int index)
{
    int previous = index - 1;
    if (index > 0 && (mem->data[index].blockSize < mem->data[previous].blockSize || (mem->data[index].blockSize == mem->data[previous].blockSize && mem->data[index].startIndex < mem->data[previous].startIndex)))
    {
        swapMemory(&mem->data[index], &mem->data[previous]);
        return shiftUpMemory(mem, previous);
    }
    return index;
}

void shiftDownMemory(Memory *mem, int index)
{
    int next = index + 1;
    int current = index;

    if (next < mem->size && (mem->data[next].blockSize < mem->data[current].blockSize || (mem->data[next].blockSize == mem->data[current].blockSize && mem->data[next].startIndex < mem->data[current].startIndex)))
        current = next;
    if (current != index)
    {
        swapMemory(&mem->data[index], &mem->data[current]);
        shiftDownMemory(mem, current);
    }
}

int pushMemory(Memory *mem, MemBlock value)
{
    if (mem->size >= MAX_MEMORY_SIZE)
        return -1;
    mem->data[mem->size++] = value;
    return shiftUpMemory(mem, mem->size - 1);
}

MemBlock removeMemory(Memory *mem, int index) // removes block at a specific index
{
    MemBlock block;
    if (mem->size <= 0)
        return block;
    block = mem->data[index];
    mem->data[index] = mem->data[mem->size - 1];
    mem->size--;
    shiftDownMemory(mem, index);
    return block;
}

Memory *createMemory()
{
    Memory *mem = (Memory *)malloc(sizeof(Memory));
    mem->size = 0;
    MemBlock root;
    initBlock(&root, 0, 1023); // initialise with 1024 memory block
    pushMemory(mem, root);
    return mem;
}

bool allocateMemory(Process *process, Memory *memory)
{
    int index = 0;
    while (index < memory->size && ((memory->data[index]).blockSize < process->memSize || (memory->data[index]).isFree == false)) // breaks from loop when met with first block accomodating the process
    {
        index++;
    }
    if (index == memory->size)
    {
        return false;
    }
    while (process->memSize <= memory->data[index].blockSize / 2) // divide block into 2 halves
    {
        MemBlock leftBlock;
        MemBlock rightBlock;
        initBlock(&leftBlock, memory->data[index].startIndex, memory->data[index].startIndex + (memory->data[index].blockSize - 1) / 2);
        initBlock(&rightBlock, memory->data[index].endIndex - (memory->data[index].blockSize - 1) / 2, memory->data[index].endIndex);
        removeMemory(memory, index);           // remove merged block to keep only leaves in queue
        index = pushMemory(memory, leftBlock); // attempt allocation in left branch first
        pushMemory(memory, rightBlock);
    }
    memory->data[index].isFree = false;
    process->memStartIndex = memory->data[index].startIndex;
    process->memEndIndex = memory->data[index].endIndex;
    return true;
}

void deallocateMemory(Process *process, Memory *memory)
{
    int index = 0;
    while (memory->data[index].startIndex != process->memStartIndex) // breaks from loop when required block is reached
    {
        index++;
    }
    while (memory->data[index].blockSize < 1024)
    {
        if (((memory->data[index]).startIndex / (memory->data[index]).blockSize) % 2 == 0) // left child
        {
            if (index + 1 < memory->size && (memory->data[index + 1].startIndex == memory->data[index].endIndex + 1) && (memory->data[index].blockSize == memory->data[index + 1].blockSize) && (memory->data[index + 1].isFree == true)) // sibling block exists and is free
            {
                MemBlock mergedBlock;
                initBlock(&mergedBlock, memory->data[index].startIndex, memory->data[index + 1].endIndex);
                removeMemory(memory, index); // remove required block
                removeMemory(memory, index); // remove sibling block
                index = pushMemory(memory, mergedBlock);
            }
            else
            {
                memory->data[index].isFree = true;
                return;
            }
        }
        else // right child
        {
            if (index > 0 && (memory->data[index - 1].endIndex + 1 == memory->data[index].startIndex) && (memory->data[index - 1].blockSize == memory->data[index].blockSize) && (memory->data[index - 1].isFree == true))
            {
                MemBlock mergedBlock;
                initBlock(&mergedBlock, memory->data[index - 1].startIndex, memory->data[index].endIndex);
                removeMemory(memory, index - 1);
                removeMemory(memory, index - 1);
                index = pushMemory(memory, mergedBlock);
            }
            else
            {
                memory->data[index].isFree = true;
                return;
            }
        }
    }
}