#include "stdint.h"
#include "memory.h"
#include "mini_uart.h"
#include "list.h"
#include "exception.h"

extern char _heap_top;
static char* heaptop_ptr = &_heap_top;
static size_t memory_size = ALLOC_END - ALLOC_BASE; // For QEMU

/* ---- Lab2 ---- */
void* malloc(size_t size) {

    // -> heaptop_ptr
    //               header 0x10 bytes                   block
    //   ┌──────────────────────┬────────────────┬──────────────────────┐
    //   │  fill zero 0x8 bytes │ size 0x8 bytes │ size padding to 0x10 │
    //   └──────────────────────┴────────────────┴──────────────────────┘
    //

    // Add heap block header 16 bytes
    char* ptr = heaptop_ptr + 0x10;
    // Size paddling to multiple of 0x10
    size = size + 0x10 - size % 0x10;
    *(unsigned int*)(ptr - 0x8) = size;
    heaptop_ptr += size;    
    return ptr;
}

/* ---- Lab4 ---- */
static frame_t          *frame_array;
static list_head_t      frame_freelist[FRAME_MAX_IDX];  // store available block for page
static list_head_t      cache_list[CACHE_MAX_IDX];      // store available block for cache

void allocator_init() {   

    // uart_puts("\r\n  ---------- | Buddy System | Startup Allocation | ----------\r\n");
    // uart_puts("\r\n  memory size: 0x%x, max pages: %d, frame size: %d\r\n", memory_size, MAX_PAGE_COUNT, sizeof(frame_t));

    frame_array = malloc(MAX_PAGE_COUNT * sizeof(frame_t));

    /* init frame array */ 
    for (int i=0; i < MAX_PAGE_COUNT; i++) {
        if (i % (1 << FRAME_IDX_FINAL) == 0) {
            frame_array[i].val  = FRAME_IDX_FINAL;
            frame_array[i].used = FRAME_VAL_FREE;
        }
        // frame_array[i].idx = i;
        // frame_array[i].order = CACHE_NONE;
    }

    /* init frame freelist */
    for (int i = 0; i <= FRAME_IDX_FINAL; i++) {
        INIT_LIST_HEAD(&frame_freelist[i]);
    }

    /* init cache list */
    for (int i = 0; i <= CACHE_IDX_FINAL; i++) {
        INIT_LIST_HEAD(&cache_list[i]);
    }

    for (int i=0; i < MAX_PAGE_COUNT; i++) {
        uart_puts("%d", frame_array[i].val);
        INIT_LIST_HEAD(&(frame_array[i].listhead));



        if (i % (1 << FRAME_IDX_FINAL) == 0) {
            list_add(&frame_array[i].listhead, &frame_freelist[FRAME_IDX_FINAL]);
        }
    }

    // uart_puts("\r\n  -----------------------------------------------------------\r\n");
    return;
}


