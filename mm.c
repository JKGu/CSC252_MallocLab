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
    "szhang73+jgu8",
    /* First member's full name */
    "Shuran Zhang",
    /* First member's email address */
    "szhang73@u.rochester.edu",
    /* Second member's full name (leave blank if none) */
    "Junkang Gu",
    /* Second member's email address (leave blank if none) */
    "jgu8@u.rochester.edu"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


//____________________________________________MACROS_____________________________________________
/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* helpers */
#define NEXT_FREEBLKP(bp) ((char*)bp) //previous pointer in segragated list
#define PREV_FREEBLKP(bp)((char*)bp+WSIZE)//next pointer in segragated list
static void *find_fit(size_t size);//9.8
static void place(void *bp, size_t asize);//9.9
void insertFreePointer(void *p);
void removeFreePointer(void *p);
void getFreeCat(size_t size)
void block_list_start = NULL;
//__________________________________________________________________________
static void *extend_heap(size_t dwords);
static void *coalesce(void *bp);

//static void place(void *bp, size_t asize);


//________________________________STATIC FUCNTIONS__________________________________________________
static void *extend_heap(size_t words){
    char *bp;
    size_t size;

/* Allocate an even number of words to maintain alignment */
     size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
     if ((long)(bp = mem_sbrk(size)) == -1)
     return NULL;

 /* Initialize free block header/footer and the epilogue header */
     PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
     PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
     PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

 /* Coalesce if the previous block was free */
     return coalesce(bp);
}

static void *coalesce(void *bp){
 size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
 size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
 size_t size = GET_SIZE(HDRP(bp));

 if (prev_alloc && next_alloc) { /* Case 1 */
 insertFreePointer(bp);
 return bp;
 }

 else if (prev_alloc && !next_alloc) { /* Case 2 */
 size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
 removeFreePointer(NEXT_FREEBLKP(bp));
 PUT(HDRP(bp), PACK(size, 0));
 PUT(FTRP(bp), PACK(size,0));
 }

 else if (!prev_alloc && next_alloc) { /* Case 3 */
 size += GET_SIZE(HDRP(PREV_BLKP(bp)));
  removeFreePointer(PREV_FREEBLKP(bp));
 PUT(FTRP(bp), PACK(size, 0));
 PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
 bp = PREV_BLKP(bp);
 }

 else { /* Case 4 */
 size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
 GET_SIZE(FTRP(NEXT_BLKP(bp)));
  removeFreePointer(NEXT_FREEBLKP(bp));
  removeFreePointer(PREV_FREEBLKP(bp));
 PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
 PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
 bp = PREV_BLKP(bp);
 }
 insertFreePointer(bp);
 return bp;
}

//find the appropriate start pointer in the segaragated list based on the size
void getFreePosition(size_t size){
    if(size<=32){
        return block_list_start;
    }else if(size<=64){
        return block_list_start+WSIZE;
    }else if(size<=128){
        return block_list_start+2*WSIZE;
    }else if(size<=256){
        return block_list_start+3*WSIZE;
    }else if(size<=512){
        return block_list_start+4*WSIZE;
    }else if(size<=1024){
        return block_list_start+5*WSIZE;
    }
}
    

//insert the block at the correct pointer in the segragated free list
void insertFreePointer(void *p){
    void *root  = getFreePosition(GET_SIZE(HDRP(p)));
    void *prev = root;
    void *next = GET(root);

    while(next!=NULL){
        if(GET_SIZE(HDRP(next))>GET_SIZE(HDRP(p))){
            break;
        }
        prev = next;
        next = GET(next);
    }

    if(prev=root){
        PUT(root,p);
        PUT(NEXT_FREEBLKP(p),next);
        PUT(PREV_FREEBLKP(p),NULL);
        if(next!=NULL){
            PUT(PREV_FREEBLKP(next),p);
        }
    }else{
        PUT(NEXT_FREEBLKP(prev),p);
        PUT(NEXT_FREEBLKP(p),next);
        PUT(PREV_FREEBLKP(p),prev);
        if(next!=NULL){
            PUT(PREV_FREEBLKP(next),p);
        }
    }

}
void removeFreePointer(void *p){
    void *root  = getFreePosition(GET_SIZE(HDRP(p)));
    void *prev = GET(PREV_FREEBLKP(p));
    void *next = GET(NEXT_FREEBLKP(p));
    //if the previous one is null
    if(prev==NULL){
        if(next!=NULL){
            PUT(PREV_FREEBLKP(next),0);
        }
        PUT(root,next);
    }else{
        if(next!=NULL){
            PUT(PREV_FREEBLKP(next),prev);
        }
        PUT(NEXT_FREEBLKP(prev),next);
    }
    PUT(NEXT_FREEBLKP(p),NULL);
    PUT(PREV_FREEBLKP(p),NULL);

}

 static void place(void* bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    if((csize-asize)>=(2*DSIZE)){
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp),PACK(csize-asize,0));
        PUT(FTRP(bp),PACK(csize-asize,0));
    }else{
        PUT(HDRP(bp),PACK(csize,1));
        PUT(HDRP(bp),PACK(csize,1));
    }
    
}

static void *find_fit(size_t asize){

}
//__________________________________________________________________________________________
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // Create the initial empty heap 
    if ((heap_listp = mem_sbrk(14*WSIZE)) == (void *)-1)
        return -1;

    //Segregated free list    
    PUT(heap_listp, 0); 
    PUT(heap_listp + (1*WSIZE), 0);
    PUT(heap_listp + (2*WSIZE), 0); 
    PUT(heap_listp + (3*WSIZE), 0); 
    PUT(heap_listp + (4*WSIZE), 0); 
    PUT(heap_listp + (5*WSIZE), 0); 
    PUT(heap_listp + (6*WSIZE), 0); 
    PUT(heap_listp + (7*WSIZE), 0); 
    PUT(heap_listp + (8*WSIZE), 0); 
    PUT(heap_listp + (9*WSIZE), 0); 
    PUT(heap_listp + (10*WSIZE), 0); 
    PUT(heap_listp + (11*WSIZE), PACK(DSIZE,1));
    PUT(heap_listp + (12*WSIZE), PACK(DSIZE,1));
    PUT(heap_listp + (13*WSIZE), PACK(0,1));  

    block_list_start = heap_listp;
    heap_listp += (12*WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
 size_t asize; /* Adjusted block size */
 size_t extendsize; /* Amount to extend heap if no fit */
 char *bp;

/* Ignore spurious requests */
 if (size == 0)
 return NULL;

 /* Adjust block size to include overhead and alignment reqs. */
 if (size <= DSIZE)
 asize = 2*DSIZE;
 else
 asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

 /* Search the free list for a fit */
 if ((bp = find_fit(asize)) != NULL) {
 place(bp, asize);
 return bp;
 }

/* No fit found. Get more memory and place the block */
 extendsize = MAX(asize,CHUNKSIZE);
 if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
 return NULL;
 place(bp, asize);
 return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
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










