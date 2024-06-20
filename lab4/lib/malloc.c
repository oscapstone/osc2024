#include "malloc.h"
#include "mini_uart.h"
#include "buddy.h"
#include "dyn_alloc.h"

extern char __end__, __HeapLimit;
static char *heapStartAddr = &__end__;
static char *heapEndAddr = &__HeapLimit;

static unsigned int reserve_fidx_list[MAX_RESERVE_NUM][2] = {0};
static unsigned int reserved_cnt = 0;

/**
 * lab 2: simple malloc
 */
void* my_malloc(int msize) {
    int avail_size = heapEndAddr - heapStartAddr;
    if(msize > avail_size) {
        uart_send_string("heap space running out\r\n");
        return (void*) 0;
    }
    char *ptr = heapStartAddr;
    heapStartAddr += msize;
    return ptr;
}

/**
 * lab 4: memory reserve
 */

void memory_init() {
    buddy_init();
    dyn_alloc_init();

    /**
     * @brief memory reservation
     * Spin tables for multicore boot   (0x0000 - 0x1000)
     * kernel stack                     (before 0x40000)
     * kernel                           (0x80000 - 0x120000) heap end
     * initramfs                        0x20000000, 4KB
     * devicetree                       0x2EFF7A00, 32KB
     */

    memory_reserve((void*)0x0, (void*)0x120000); // spin tables, kernel image
    memory_reserve((void*)0x20000000, (void*)0x20001000); // config.txt hard coded initramfs
    memory_reserve((void*)0x2EFF7A00, (void*)0x2EFFFA00);

    process_memory_reserve();
}

void* malloc(unsigned int msize) {
    unsigned int frame_num;
    if(msize <= MAX_CHUNK_SIZE) { return alloc_chunk(msize); }
    else {
        // pad msize to 4096
        for(frame_num = 0; frame_num < (1<<MAX_ORDER); frame_num++) {
            if(frame_num * FRAME_SIZE >= msize) { return alloc_frame(frame_num); }
        }
    }
    uart_send_string("[ERROR] malloc exceed max size\r\n");
    return (void*)0;
}

void memory_reserve(void *start_addr, void *end_addr) {
    unsigned int start_fidx, end_fidx;

    if(reserved_cnt >= MAX_RESERVE_NUM) {
        uart_send_string("[ERROR] memory reserve exceed max number\r\n");
        return;
    }

    uart_send_string("[INFO] Memory reserve addr = ");
    uart_send_hex((unsigned long) start_addr);
    uart_send_string(" - ");
    uart_send_hex((unsigned long) end_addr);
    uart_send_string("\r\n");

    start_fidx = addr_to_fidx(start_addr);
    end_fidx = addr_to_fidx(end_addr);
    // addr_to_fidx is floor(), so end_fidx should be inclusive in reservation
    if((unsigned long)end_addr & 0xfff == 0) { end_fidx--; }
    reserve_fidx_list[reserved_cnt][0] = start_fidx;
    reserve_fidx_list[reserved_cnt][1] = end_fidx;    
    reserved_cnt++;
}

void process_memory_reserve() {
    for(int i = 0; i < reserved_cnt; i++) {
        reserve_fidx_range(reserve_fidx_list[i][0], reserve_fidx_list[i][1], MAX_ORDER);
    } 
    
    uart_send_string("\r\n[INFO] Show Reservation\r\n");
    dump_reservation();
}