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

#define LEFT_P(bp) ((char *)(bp))
#define RIGHT_P(bp) ((char *)(bp) + WSIZE)
#define PARENT_P(bp) ((char *)(bp) + DSIZE)

#define GET_LEFT_P(bp) GET(LEFT_P(bp))
#define GET_RIGHT_P(bp) GET(RIGHT_P(bp))
#define GET_PARENT_P(bp) GET(PARENT_P(bp))

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static void *extend_heap(size_t);
static void *coalesce(void *);
static void *find_fit(size_t);
static void *first_fit(size_t);
static void place(void *, size_t);
static int mm_check(void);

void InorderTreeWalk(char *);
static char *SearchFreeBlock(char *, size_t);
static char *FindMinimum(char *);
static void TransplantFreeBlock(char *, char *);
static void DeleteFreeBlock(char *);
static void InsertFreeBlock(char *);

static char *heap_listp;
static char *next_fit_ptr;
static char *free_root_bp;
static char *free_nil_bp;
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    #ifdef DEBUG
        printf("\n-----DEBUG MODE------\n");
    #endif
    if ((heap_listp=mem_sbrk(10*WSIZE)) == (void *)-1)
        return -1;

    // unused
    PUT(heap_listp, 0);

    // free nil
    PUT(heap_listp+(1*WSIZE), PACK(3*WSIZE, 1)); // nil header
    PUT(heap_listp+(2*WSIZE), 0); // left
    PUT(heap_listp+(3*WSIZE), 0); // right
    PUT(heap_listp+(4*WSIZE), 0); // parent
    PUT(heap_listp+(5*WSIZE), 0); // padding
    PUT(heap_listp+(6*WSIZE), PACK(3*WSIZE, 1)); // nil footer

    //prologue
    PUT(heap_listp+(7*WSIZE), PACK(DSIZE, 1)); //header
    PUT(heap_listp+(8*WSIZE), PACK(DSIZE, 1)); //footer

    //eplilogue
    PUT(heap_listp+(9*WSIZE), PACK(0, 1)); 

    free_nil_bp = heap_listp+(2*WSIZE); // nil 가리킴
    
    heap_listp+=(8*WSIZE);

    free_root_bp = free_nil_bp; // free_list_root가 nil 가리키게

    #ifdef DEBUG
        printf("heap_listp %p\n", heap_listp);
        printf("free_root_bp %p\n", free_root_bp);
        printf("free_nil_bp %p\n", free_nil_bp);
    #endif

    // free_nil_bp left, right, parent 세팅
    PUT(LEFT_P(free_nil_bp), NULL); // nil의 왼쪽
    PUT(RIGHT_P(free_nil_bp), NULL); // nil의 오른쪽
    PUT(PARENT_P(free_nil_bp), NULL); // nil의 부모
    
    
    #ifdef DEBUG
        printf("\n초기화\n");
        mm_check();
    #endif

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
    #ifdef DEBUG
        printf("\nmalloc, %d\n",size); //debug
    #endif
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
        
        place(bp, asize);

        #ifdef DEBUG
            printf("들어갈 자리 찾음\n");
            mm_check();
        #endif
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){
        #ifdef DEBUG
            printf("extend error\n");
        #endif
        return NULL;
    }
    place(bp, asize);
    #ifdef DEBUG
        printf("들어갈 자리 못찾음 -> 힙 확장 %d\n",extendsize); //debug
        mm_check();
    #endif
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    
    coalesce(bp);
    //add 따로 안해줘도 됨(coalsece에서 함)

    #ifdef DEBUG
        printf("\n해제 %d\n", size);
        mm_check();
    #endif
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{

    #ifdef DEBUG
        printf("\n재할당 %d\n", size);
    #endif

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

    PUT(HDRP(bp), PACK(size, 0)); //freeblock header
    PUT(FTRP(bp), PACK(size, 0)); //freeblock footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //에필로그
    
    #ifdef DEBUG
        printf("extend heap bp: %p\n",bp);
        mm_check();
    #endif
    //add 따로 안해줘도 됨(coalesce에서 함)
    return coalesce(bp);
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    // CASE 1 : 이전 블록 - allocated / 다음 블록 - allocated
    if(prev_alloc && next_alloc){
        //add. 아래에서 처리
    }
    // CASE 2 : 이전 블록 - allocated / 다음 블록 - free
    else if(prev_alloc && !next_alloc){
        
        DeleteFreeBlock(NEXT_BLKP(bp));

        // 현재 블록(free)와 다음 블록(free)를 연결
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0)); 
        PUT(FTRP(bp), PACK(size, 0));

    }
    // CASE 3 : 이전 블록 - free / 다음 블록 - allocated
    else if (!prev_alloc && next_alloc){

        DeleteFreeBlock(PREV_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

    }
    // CASE 4 : 이전 블록 - free / 다음 블록 - free
    else{

        DeleteFreeBlock(NEXT_BLKP(bp));
        DeleteFreeBlock(PREV_BLKP(bp));
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp=PREV_BLKP(bp);

    }
    InsertFreeBlock(bp);

    #ifdef DEBUG
        printf("\ncoalescing 이후\n");
        mm_check();
    #endif

    return bp;
}

/* Helper function */

static void *find_fit(size_t asize)
{
    return (void *)SearchFreeBlock(free_root_bp, asize);
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
    // 남은 free block이 4 words 미만일 때
    if((base_size - asize) < 4 * WSIZE){
        PUT(FTRP(bp), PACK(base_size, 1));
        PUT(HDRP(bp), PACK(base_size, 1));
        
    }
    // 남은 free block이 4 words 이상일 때
    else{  //split
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(base_size-asize, 0)); //암시적으로 넥스트로 감
        PUT(FTRP(NEXT_BLKP(bp)), PACK(base_size-asize, 0));
        
        #ifdef DEBUG
            printf("\n분할\n");
            mm_check();
        #endif

        InsertFreeBlock(NEXT_BLKP(bp));
    }
    DeleteFreeBlock(bp);
    return;
}

int mm_check(void){

    alloc_t allocated;
    size_t size;

    char *bp = mem_heap_lo()+2*WSIZE;
    
    printf("\nmm_check\n");
    printf("mem_start_brk %p\n", mem_heap_lo());
    printf("mem_brk %p\n", mem_heap_hi());
    printf("mem_heap_size %d\n", mem_heapsize());

    int i=0;

    //전체 블록 추적
    printf("\n 전체 블록 출력 \n");
    printf("----프리리스트 헤드, 테일----\n");
    while(bp < (char *)mem_heap_hi()+1)
    {
        allocated = GET_ALLOC(HDRP(bp));
        size = GET_SIZE(HDRP(bp));
        printf("[%d] bp:%p, size:%d alloc:%ld, \n    HDRp:%p, FTRp:%p,  \n    NEXTp:%p\n",
            i,bp, size, allocated, HDRP(bp), FTRP(bp), NEXT_BLKP(bp)
        );
        printf("    bp-hdrp:%d,ftrp-hdrp:%d,nextp-ftrp:%d\n\n",
            bp-HDRP(bp), FTRP(bp)-HDRP(bp), NEXT_BLKP(bp)-FTRP(bp)
        );
        bp = NEXT_BLKP(bp);
        i++;
        if(i==2){
            printf("---프롤로그 부터---\n");
        }
    }
    
    i=0;
    bp = free_root_bp;
    printf("\n free list 출력 \n");
    InorderTreeWalk(bp);
    //free list 추적
    return 0;
}

/* 중위 순회 */
void InorderTreeWalk(char *bp)
{   
    alloc_t allocated;
    size_t size;
    
    if (bp != NULL)
    {
        InorderTreeWalk(GET_LEFT_P(bp));

        allocated = GET_ALLOC(HDRP(bp));
        size = GET_SIZE(HDRP(bp));

        printf("[*] bp:%p, size:%d alloc:%ld, \n    HDRp:%p, FTRp:%p, \n    LEFTp:%p RIGTp:%p\n",
            bp, size, allocated, HDRP(bp), FTRP(bp), GET_LEFT_P(bp), GET_RIGHT_P(bp)
        );
        printf("    bp-hdrp:%d,ftrp-hdrp:%d\n\n",
            bp-HDRP(bp), FTRP(bp)-HDRP(bp)
        );

        InorderTreeWalk(GET_RIGHT_P(bp));
    }
    return;

}

/* 노드 검색 */
char *SearchFreeBlock(char *bp, size_t size)
{
    char *current = bp;
    if (GET_ALLOC(HDRP(current)) || size == GET_SIZE(HDRP(current)))
        return current;
    if (size < GET_SIZE(HDRP(current)))
        return SearchFreeBlock(GET_LEFT_P(bp), size);
    else
        return SearchFreeBlock(GET_RIGHT_P(bp), size);
}

/* 최소 원소 반환 */
char *FindMinimum(char *bp)
{
    while (!GET_ALLOC(HDRP(GET_LEFT_P(bp))))
        bp = GET_LEFT_P(bp);
    return bp;
}

/* 이식 */
void TransplantFreeBlock(char *old_bp, char *new_bp)
{
    if (GET_ALLOC(GET_PARENT_P(old_bp)))
        free_root_bp = new_bp;
    else if (old_bp == GET_LEFT_P(GET_PARENT_P(old_bp)))
        PUT(LEFT_P(GET_PARENT_P(old_bp)), new_bp);
    else
        PUT(RIGHT_P(GET_PARENT_P(old_bp)), new_bp);
    if (!GET_ALLOC(new_bp))
        PUT(PARENT_P(new_bp), GET_PARENT_P(old_bp));
}

/* 삭제 */
void DeleteFreeBlock(char *bp)
{
    char *y;
    if (GET_ALLOC(GET_LEFT_P(bp)))
        TransplantFreeBlock(bp, GET_RIGHT_P(bp));
    else if (GET_ALLOC(GET_RIGHT_P(bp)))
        TransplantFreeBlock(bp, GET_LEFT_P(bp));
    else{
        y = FindMinimum(GET_RIGHT_P(bp));
        if (GET_PARENT_P(y) != bp){
            TransplantFreeBlock(y, GET_RIGHT_P(y));
            PUT(RIGHT_P(y), GET_RIGHT_P(bp));
            PUT(PARENT_P(GET_RIGHT_P(y)), y);
        }
        TransplantFreeBlock(bp, y);
        PUT(LEFT_P(y), GET_LEFT_P(bp));
        PUT(PARENT_P(GET_LEFT_P(y)) ,y);
    }
}

/* 삽입 */
void InsertFreeBlock(char *bp)
{
    char *y = free_nil_bp;
    char *x = free_root_bp;
    while (!GET_ALLOC(x))
    {
        y = x;  // y에 x 이전 값 계속 저장
        if (GET_SIZE(HDRP(bp)) < GET_SIZE(HDRP(x)))
            PUT(x, GET_LEFT_P(x));
        else
            PUT(x, GET_RIGHT_P(x));
    }
    PUT(PARENT_P(bp), y);
    // printf("%d\n", node->parent->key);
    if (GET_ALLOC(y))
        free_root_bp = bp;
    else if (GET_SIZE(HDRP(bp)) < GET_SIZE(HDRP(y)))
        PUT(LEFT_P(y), bp);
    else
        PUT(RIGHT_P(y), bp);
}