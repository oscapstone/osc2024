#ifndef MM_H
#define MM_H

#include "base.h"
#include "arm/mmu.h"

#define MEM_FRAME_FLAG_UNUSED   0
#define MEM_FRAME_FLAG_USED     1

typedef struct _frame_info
{
    U8 flag;
} frame_info;

typedef struct _memory_manager {
    U64 base_ptr;                   // the memory base address
    U64 size;                       // the size of memory
    struct frame_info *frames;             // the pointer of the start frame info
} memory_manager;

#define MEM_FRAME_SIZE          PD_PAGE_SIZE

void mm_init();

// mmutiilASM
void memzero(unsigned long src, U32 n);



#endif


