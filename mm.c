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

/**
 * Choices:
 *
 * Size stored in header = entire length of block (incl. header and footer)
 * block basepointer = points to start of data (not start of footer)
 * size of header and footer = one word
 */

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE mem_pagesize()

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define IS_ALLOC(p) (GET(p) & 0x1)

/* HDRP computes the address of the given pointers block header */
#define HDRP(bp) ((char *)(bp)-WSIZE)

/** Computes the address to the footer of a block */
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

#define GET_PREV_FOOTER(bp) ((char *)(bp)-DSIZE)

/* single word (4) or double word (8) alignment */
#define ALIGNMENT DISZE

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

void *heap_listp;

/**
 * Attempts to coalese the given block (pointer) with the previous and next block,
 * in order to keep free blocks as long as possible.
 */
static void *coalesce(void *bp)
{
    char *next_hdr = HDRP(NEXT_BLKP(bp));
    size_t prev_alloc = IS_ALLOC(GET_PREV_FOOTER(bp));
    size_t next_alloc = IS_ALLOC(next_hdr);
    size_t size = GET_SIZE(HDRP(bp));

    /** If prev and next is both allocated then we cannot do anything. */
    if (prev_alloc && next_alloc)
    {
        return bp;
    }

    /** If prev is allocated and next is free, merge with next block */
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(next_hdr);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    }

    /** Same as above, however with the prev block free and the next block allocated */
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(GET_PREV_FOOTER(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    /** If both prev and next are free blocks, then merge all together to on big block */
    else
    {
        size += GET_SIZE(GET_PREV_FOOTER(bp)) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}

/**
 * Internal helper that extends the heap by the amount of words specified
 */
static void *extend_heap(size_t words)
{
    /** Allocate even number of words to maintain alignment */
    size_t size = ((words % 2) ? (words + 1) : words) * DSIZE;
    char *bp = mem_sbrk(size);

    if (bp == (char *)-1)
    {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    return (void *)coalesce(bp);
}

/**
 * Helper that traverses the implicit freelist and returns the first block that
 * is free and is larger than or equal to the size requested
 */
static void *find_fit(size_t size)
{
    char *ptr = heap_listp;

    while (GET_SIZE(HDRP(ptr)))
    {
        char *hp = HDRP(ptr);
        int blockSize = GET_SIZE(hp);

        // In case we've found a block that is larger than the requested since
        // and isn't allocated, then return the block to that pointer!
        if (!IS_ALLOC(hp) && blockSize >= size)
        {
            return (void *)ptr;
        }

        // Move to next block!
        ptr += blockSize;
    }

    return NULL;
}

/**
 * Places a block at the given pointer with a given size
 */
void place(void *ptr, size_t size)
{
    size_t oldSize = GET_SIZE(HDRP(ptr));

    // Update old free block footer and new header
    PUT(HDRP(ptr + size), PACK(oldSize - size, 0));
    PUT(FTRP(ptr), PACK(oldSize - size, 0));
    // Update new block to be allocated
    PUT(HDRP(ptr), PACK(size, 1));
    PUT(FTRP(ptr), PACK(size, 1));
}

/**
 * Diagnostics function that checks all blocks in the heap, including the prologue-
 * block, but stops at the epilogue block
 */
int mm_check(void)
{
    int blocks = 0;
    int blocksSize = 0;
    int free = 0;
    int freeSize = 0;
    int notCoalesced = 0;

    char *ptr = heap_listp;

    while (GET_SIZE(HDRP(ptr)))
    {
        // Gather diagnostics regarding the current block
        char *hp = HDRP(ptr);
        int size = GET_SIZE(hp);
        blocks++;
        blocksSize += size;

        if (!IS_ALLOC(hp))
        {
            free++;
            freeSize += size;
            size_t prev_alloc = IS_ALLOC(GET_PREV_FOOTER(ptr));
            size_t next_alloc = IS_ALLOC(HDRP(NEXT_BLKP(ptr)));

            if (!prev_alloc || !next_alloc)
            {
                notCoalesced++;
            }
        }

        // Move to next block!
        ptr += size;
    }

    printf("Heapsize: %i bytes. Not using %i bytes\n%i blocks (size = %i), including proglogue in the heap\n%i blocks are free (size = %i)\n%i blocks were not coalesced.\n\n", mem_heapsize(), (mem_heapsize() - blocksSize), blocks, blocksSize, free, freeSize, notCoalesced);

    return 0;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create initial empty heap */
    heap_listp = mem_sbrk(4 * WSIZE);

    if (heap_listp == (void *)-1)
    {
        return -1;
    }

    PUT(heap_listp, 0);                            /** Allignment padding */
    PUT(heap_listp + (WSIZE), PACK(DSIZE, 1));     /** Prologue header*/
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /** Prologue footer*/
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /** Epilogue header */
    heap_listp += (WSIZE * 2);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / DSIZE) == (void *)-1)
    {
        return -1;
    }

    mm_check();
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // Ignore irrelevant requests
    if (size == 0)
    {
        return NULL;
    }

    size_t asize;      // Adjusted block size (i.e. aligned)
    size_t extendsize; // Size we need to extend to make room for the requested size
    char *ptr;

    // Adjust block size to include overhead (header and footer) and alignment
    if (size <= DSIZE)
    {
        // the smallest we can assign is 16 bytes (8 for header and fotter and 8 for content + padding)
        asize = 2 * DSIZE;
    }
    else
    {
        // Round down to the lowest amount of bytes we need while maintaining alignment + overhead
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    }

    // Search the free list for a fit
    ptr = find_fit(asize);
    if (ptr == NULL)
    {
        // If none, extend the heap to a fitting size
        extendsize = MAX(asize, CHUNKSIZE);

        if ((ptr = extend_heap(extendsize / DSIZE)) == NULL)
        {
            return NULL;
        }
    }

    place(ptr, asize);
    printf("alloc %i %i %i\n", size, asize, extendsize);
    mm_check();
    return ptr;
}

/*
 * Frees the block at the given pointer, by flipping the allocated fields to 0
 * and afterwards attempts to coalesce the prev and next block.
 */
void mm_free(void *ptr)
{
    printf("free\n");

    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
    mm_check();
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *newPtr;
    if ((newPtr = mm_malloc(size)) == NULL)
    {
        return NULL;
    }

    size_t copySize = GET_SIZE(HDRP(ptr)) - DSIZE;

    // If we're shrinking, make sure we don't copy
    // more than necessary to avoid errors
    if (size < copySize)
    {
        copySize = size;
    }

    memcpy(newPtr, ptr, copySize);
    mm_free(ptr);
    printf("Realloc\n");
    mm_check();
    return newPtr;
}
