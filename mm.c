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
typedef unsigned long alloc_t;

team_t team = {
    /* Team name */
    "team-fundamental",
    /* First member's full name */
    "So KyungHyun",
    /* First member's email address */
    "valentine92@gmail.com",
    /* Second and Third member's full name (leave blank if none) */
    "Kim Jintae, Jo youjin",
    /* Second and Third member's email address (leave blank if none) */
    "realbig4199@gmail.com, youjijoy@gmail.com"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x, y) ((x) > (y) ? (x):(y))
#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static void *extend_heap(size_t);
static void *coalesce(void *);
static void *find_fit(size_t);
static void *first_fit(size_t);
static void place(void *, size_t);
static char *heap_listp;
int mm_check(void);
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp=mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp+(1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp+(2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp+(3*WSIZE), PACK(0, 1));
    heap_listp+=(2*WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE)==NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */


void *mm_malloc(size_t size)
{
    //mm_check();
    // printf("malloc, %d\n",size);
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    if ((bp = find_fit(asize)) != NULL){
        // printf("들어갈 자리 찾음\n");
        place(bp, asize);
        // printf("bp: %p size: %d\n",bp,GET_SIZE(HDRP(bp)));
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    // printf("a:%d, c:%d, e:%d\n",asize, CHUNKSIZE, extendsize);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){
        // printf("extend error\n");
        return NULL;
    }
    // printf("들어갈 자리 못찾음 -> 힙 확장 %d\n",extendsize);
    place(bp, asize);
    // printf("bp: %p size: %d\n",bp,GET_SIZE(HDRP(bp)));
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    // printf("free %p\n",HDRP(bp));

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


static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // CASE 1 : 이전 블록 - allocated / 다음 블록 - allocated
    if(prev_alloc && next_alloc){
        return bp;
    }
    // CASE 2 : 이전 블록 - allocated / 다음 블록 - free
    else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0)); 
        PUT(FTRP(bp), PACK(size, 0));
    }
    // CASE 3 : 이전 블록 - free / 다음 블록 - allocated
    else if (!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    // CASE 4 : 이전 블록 - free / 다음 블록 - free
    else{
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp=PREV_BLKP(bp);
    }
    return bp;
}

/* Helper function */

static void *find_fit(size_t asize)
{
    return first_fit(asize);
}

static void* first_fit(size_t asize)
{
    alloc_t allocated;
    size_t size;

    char *bp = heap_listp;

    while(bp < (char *)mem_heap_hi()+1)
    {
        allocated = GET_ALLOC(HDRP(bp));
        size = GET_SIZE(HDRP(bp));
        
        if (!allocated && size >= asize)
            return bp;
        bp = NEXT_BLKP(bp);
    }
    return NULL;

}

static void place(void *bp, size_t asize){
    size_t base_size = GET_SIZE(HDRP(bp));
    // printf("할당 중 %p \n할당size: %d 원래size:%d\n", bp, asize, base_size);
    // 남은 free block이 4 words 미만일 때
    if((base_size - asize) < 4 * WSIZE){
        // printf("내부단편화로 할당\n");
        // 남은 free block까지 포함해서 내부적 단편화(internal fragmentation)하기
        PUT(FTRP(bp), PACK(base_size, 1));
        PUT(HDRP(bp), PACK(base_size, 1));
    }
    // 남은 free block이 4 words 이상일 때
    else{
        // printf("새 블록 쪼개면서 할당\n");
        // allocated block
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        // printf("앞 hdr:%p size%d ftr%p\n",
        //     HDRP(bp), GET_SIZE(HDRP(bp)), FTRP(bp)
        // );
        // free block
        PUT(HDRP(NEXT_BLKP(bp)), PACK(base_size-asize, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(base_size-asize, 0));
        // printf("뒤 hdr:%p size%d ftr%p\n",
        //     HDRP(NEXT_BLKP(bp)), GET_SIZE(HDRP(NEXT_BLKP(bp))), 
        //     FTRP(NEXT_BLKP(bp))
        // );
    }
}

int mm_check(void){

    alloc_t allocated;
    size_t size;

    char *bp = heap_listp;
    // printf("\nmm_check\n");
    int i=0;
    while(bp < (char *)mem_heap_hi()+1)
    {
        allocated = GET_ALLOC(HDRP(bp));
        size = GET_SIZE(HDRP(bp));
        // printf("[%d] bp:%p, size:%d alloc:%ld, \n  hdrp:%p, ftrp:%p,  \n nxtp:%p\n",
        //     i,bp, size, allocated, HDRP(bp), FTRP(bp), NEXT_BLKP(bp)
        // );
        // printf("  bp-hdrp:%d,ftrp-hdrp:%d,nextp-ftrp:%d\n\n",
        //     bp-HDRP(bp), FTRP(bp)-HDRP(bp), NEXT_BLKP(bp)-FTRP(bp)
        // );
        bp = NEXT_BLKP(bp);
        i++;
    }
    return 0;
}
