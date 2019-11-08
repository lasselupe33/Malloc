/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "group103",
    /* First member's full name */
    "Lasse Agersten",
    /* First member's email address */
    "lage@itu.dk",
    /* Second member's full name (leave blank if none) */
    "Christina Steinhauer",
    /* Second member's email address (leave blank if none) */
    "stei@itu.dk"};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

typedef long long word;

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define Length(hdr) (((hdr) >> 1))
#define IsAllocated(hdr) ((hdr)&1)

word mkheader(unsigned int allocated, unsigned int length)
{
    return (allocated) | (length << 1);
}

int mm_check(void)
{
    word *startPtr = mem_heap_lo();
    word *ptr = startPtr;
    word *endPtr = mem_heap_hi();
    //printf("Called: %p %p", ptr, mem_heap_hi());

    int blocks = 0;
    int blockSize = 0;
    int free = 0;
    int freeSize = 0;

    while (ptr <= endPtr)
    {
        if (Length(ptr[0]) > 0)
        {
            blocks++;
            blockSize += Length(ptr[0]);
        }

        if (!IsAllocated(ptr[0]))
        {
            free++;
            freeSize += Length(ptr[0]);
        }

        word *nextblock = ptr + Length(ptr[0]) / ALIGNMENT;

        if (nextblock > (endPtr + 1))
        {
            printf("Block at heap[%i] extends out of heap\n\n", (ptr - startPtr));
            exit(1);
        }

        ptr = nextblock;
    }

    printf("%i blocks (size = %i) in the heap\n%i blocks are free (size = %i)\n\n", blocks, blockSize, free, freeSize);

    return 0;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    mem_sbrk(8 * ALIGNMENT);

    word *heapStart = mem_heap_lo();
    word *heapEnd = mem_heap_hi();

    heapStart[0] = mkheader(0, mem_heapsize());
    heapEnd[0] = mkheader(0, mem_heapsize());

    mm_malloc(2);

    //mm_check();

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    void *p = mem_heap_lo();
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}