#ifndef MM_H
#define MM_H

#include "base.h"
#include "arm/mmu.h"
#include "proc/task.h"

#define MEM_FREE_INFO_UNUSED        ((U32)-1)

typedef struct _FREE_INFO {
    U32 space;                   // the max size of info list
    U32 size;                    // the current size of this free list
    /**
     * contain the first frame idx for this free size
    */
    U32* info;                  // info list
}FREE_INFO;

#define MEM_FRAME_NOT_FOUND         MEM_FREE_INFO_UNUSED
    
// the maxinum support memory size is 256G the maximum size in frame array is 16(0x10)
#define MEM_FRAME_FLAG_USED          0x01
#define MEM_FRAME_FLAG_CONTINUE      0x02
#define MEM_FRAME_FLAG_CANT_USE      0x04               // mark by memory reserve
#define MEM_FRAME_FLAG_CHUNK         0x08               // is this frame allocated by chunk?

#define MEM_FRAME_IS_INUSE(frame)   (frame.flag & MEM_FRAME_FLAG_USED)
#define MEM_FRAME_IS_CHUNK(frame)   (frame.flag & MEM_FRAME_FLAG_CHUNK)

typedef struct _FRAME_INFO {
    /**
     * bit order
     * 0: whether it is allocated
     * 1: whether it is continue (not the first block)
     * 2: whether it can not be allocated
    */
    U8 flag;
    U8 order;           // the order of the frame in buddy system
    U8 pool_order;     // if used by chunk where is the order pool
    U8 ref_count;       // this frame reference count
}FRAME_INFO;

#define MEM_FRAME_INFO_SIZE         sizeof(FRAME_INFO)

typedef struct _CHUNK_SLOT {
    U8 flag;                        // the flag of this slot
}CHUNK_SLOT;

#define MEM_CHUNK_SLOT_FLAG_USED    0x01

typedef struct _CHUNK_INFO {
    U32 frame_index;
    U32 ref_count;                      // current slot being used
    CHUNK_SLOT slots[256];              // allocated by memory manager not using static allocator
}CHUNK_INFO;

#define MEM_MAX_CHUNK               100

typedef struct _MEMORY_POOL
{
    U8 obj_size;                    // a small size this pool using (in byte)
    U32 slot_size;                  // how many slot can insert to this order pool
    U8 chunk_count;                // current chunk allocate in this pool
    CHUNK_INFO* chunks;             
}MEMORY_POOL;

#define MEM_MIN_OBJECT_SIZE         16
// the order of the pools 
// 4 mean 16 = 0, 32 = 1, 48 = 2, 64 = 3, 80 = 4, 96 = 5 in pool list
#define MEM_MEMORY_POOL_ORDER       6

typedef struct _MEMORY_MANAGER {
    U64                     base_ptr;                   // the memory base address
    U64                     size;                       // the size of memory
    U32                     levels;                     // the max level (exp) of the memory

    U32                     number_of_frames;           // number of frames in memory
    FRAME_INFO*             frames;                     // the array of frame

    /**
     * free_list[0] for 4k
     * free_list[1] for 8k
     * ... etc
    */
    FREE_INFO* free_list;

    MEMORY_POOL* pools;

} MEMORY_MANAGER;

#define MEM_FRAME_SIZE          PD_PAGE_SIZE

void mm_init();

U64 mem_addr2idx(UPTR x);
UPTR mem_idx2addr(U32 x);

void* kmalloc(U64 size);
void kfree(void* ptr);
/**
 * Allocate and zero the memory
*/
void* kzalloc(U64 size);


// mmutiilASM
void memzero(void* src, U32 n);
void memcpy(const void* src, void* dst, size_t size);



#endif


