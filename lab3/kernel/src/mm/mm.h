#ifndef MM_H
#define MM_H

#include "base.h"
#include "arm/mmu.h"

#define MEM_FREE_INFO_UNUSED        ((U32)-1)

typedef struct _FREE_INFO {
    U32 size;                   // the max size of info list
    /**
     * contain the first frame idx for this free size
    */
    U32* info;                  // info list
}FREE_INFO;

#define MEM_FRAME_NOT_FOUND         MEM_FREE_INFO_UNUSED
    
// the maxinum support memory size is 256G the maximum size in frame array is 16(0x10)
#define MEM_FRAME_FLAG_CONTINUE      0x02
#define MEM_FRAME_FLAG_USED          0x01
#define MEM_FRAME_FLAG_CANT_USE      0x04

typedef struct _FRAME_INFO {
    /**
     * bit order
     * 0: whether it is allocated
     * 1: whether it is continue (not the first block)
     * 2: whether it can not be allocated
    */
    U8 flag;
    U8 order;           // the order of the frame
}FRAME_INFO;

#define MEM_FRAME_INFO_SIZE         sizeof(FRAME_INFO)

typedef struct _MEMORY_MANAGER {
    U64                     base_ptr;                   // the memory base address
    U64                     size;                       // the size of memory
    U32                     levels;                     // the max level (exp) of the memory

    U32                     number_of_frames;           // 
    FRAME_INFO*             frames;                    // the array of frame

    /**
     * free_list[0] for 4k
     * free_list[1] for 8k
     * ... etc
    */
    FREE_INFO* free_list;

} MEMORY_MANAGER;



#define MEM_FRAME_SIZE          PD_PAGE_SIZE

void mm_init();

// mmutiilASM
void memzero(void* src, U32 n);



#endif


