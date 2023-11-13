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
    /* Second member's full name and email address (leave blank if none) */
    "Kim Jintae: realbig4199@gmail.com",
    /* Third member's full name and email address (leave blank if none) */
    "Jo youjin: youjinjoy@gmail.com"
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

#define PRED(bp) ((char *)(bp))
#define SUCC(bp) ((char *)(bp) + WSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static void *extend_heap(size_t);
static void *coalesce(void *);
static void *find_fit(size_t);
static void *first_fit(size_t);
static void *next_fit(size_t);
static void place(void *, size_t);
static void add_explicit_free_block(char *);
static void splice_explicit_free_block(char *);
static int mm_check(void);

static char *heap_listp;
static char *next_fit_ptr;
static char *free_list_head;
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
    next_fit_ptr = heap_listp;
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
    
    // printf("malloc, %d\n",size); //debug
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
        // printf("들어갈 자리 찾음\n");  //debug
        place(bp, asize);
        mm_check();
        // printf("bp: %p size: %d\n",bp,GET_SIZE(HDRP(bp)));
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    // printf("a:%d, c:%d, e:%d\n",asize, CHUNKSIZE, extendsize);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){
        // printf("extend error\n");
        return NULL;
    }
    // printf("들어갈 자리 못찾음 -> 힙 확장 %d\n",extendsize); //debug
    place(bp, asize);
    mm_check();
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
    next_fit_ptr = coalesce(bp);
    // printf("해제\n"); //debug
    mm_check();
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
    copySize = GET_SIZE(HDRP(oldptr)) - DSIZE;
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

void add_explicit_free_block(char * bp){

    char *first_node = GET(free_list_head); // 연결 리스트(explicit free list)의 첫 번째 노드

    // head 와 bp의 관계
    PUT(free_list_head, bp);// head의 successor 에 bp 넣기
    PUT(PRED(bp), free_list_head); // bp의 predecessor에 head 주소 넣기
    // bp 와 first_node의 관계
    PUT(SUCC(bp), first_node); // bp의 successor에 first_node 넣기
    PUT(PRED(first_node), bp); // first_node의 predecessor에 bp 포인터 넣기

}

void splice_explicit_free_block(char * bp){

    char *prev_node = PRED(bp);
    char *next_node = SUCC(bp);

    // 다음 블록(free)에 대한 연결 해제
    PUT(SUCC(prev_node), next_node);
    PUT(PRED(next_node), prev_node);

}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    

    // CASE 1 : 이전 블록 - allocated / 다음 블록 - allocated
    if(prev_alloc && next_alloc){

        add_explicit_free_block(bp);

        return bp;
    }
    // CASE 2 : 이전 블록 - allocated / 다음 블록 - free
    else if(prev_alloc && !next_alloc){
        
        splice_explicit_free_block(NEXT_BLKP(bp));

        // 현재 블록(free)와 다음 블록(free)를 연결
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0)); 
        PUT(FTRP(bp), PACK(size, 0));
        
        add_explicit_free_block(bp);

    }
    // CASE 3 : 이전 블록 - free / 다음 블록 - allocated
    else if (!prev_alloc && next_alloc){

        splice_explicit_free_block(PREV_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

        add_explicit_free_block(bp);
    }
    // CASE 4 : 이전 블록 - free / 다음 블록 - free
    else{
        splice_explicit_free_block(NEXT_BLKP(bp));
        splice_explicit_free_block(PREV_BLKP(bp));
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp=PREV_BLKP(bp);

        add_explicit_free_block(bp);
    }

    return bp;
}

/* Helper function */

static void *find_fit(size_t asize)
{
    // return first_fit(asize);
    return next_fit(asize);

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

static void* next_fit(size_t asize)
{   
    alloc_t allocated;
    size_t size;

    char *bp = next_fit_ptr;

    while(bp < (char *)mem_heap_hi()+1)
    {
        allocated = GET_ALLOC(HDRP(bp));
        size = GET_SIZE(HDRP(bp));
        
        if (!allocated && size >= asize){
            return bp;
        }
        bp = NEXT_BLKP(bp);
    }
    char *last_bp = PREV_BLKP(bp);
    bp = heap_listp;
    while(bp < next_fit_ptr)
    {
        allocated = GET_ALLOC(HDRP(bp));
        size = GET_SIZE(HDRP(bp));
        
        if (!allocated && size >= asize){
            return bp;
        }
        bp = NEXT_BLKP(bp);
    }
    next_fit_ptr = last_bp;
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
    next_fit_ptr = NEXT_BLKP(bp);
    return;
}

int mm_check(void){
    return 0;

    alloc_t allocated;
    size_t size;

    char *bp = heap_listp;
    // printf("\nmm_check\n");
    int i=0;
    while(bp < (char *)mem_heap_hi()+1)
    {
        allocated = GET_ALLOC(HDRP(bp));
        size = GET_SIZE(HDRP(bp));
        printf("[%d] bp:%p, size:%d alloc:%ld, \n  hdrp:%p, ftrp:%p,  \n nxtp:%p\n",
            i,bp, size, allocated, HDRP(bp), FTRP(bp), NEXT_BLKP(bp)
        );
        printf("  bp-hdrp:%d,ftrp-hdrp:%d,nextp-ftrp:%d\n\n",
            bp-HDRP(bp), FTRP(bp)-HDRP(bp), NEXT_BLKP(bp)-FTRP(bp)
        );
        bp = NEXT_BLKP(bp);
        i++;
    }
    return 0;
}
