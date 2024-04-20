#include "uart.h"

#define MAX_ORDER 5
#define PAGE_SIZE 4096  // Assuming a page size of 4KB
#define MEMORY_START 0x10000000
#define MEMORY_SIZE 0x10000000  // Managing 256MB of memory
#define FRAME_COUNT (MEMORY_SIZE / PAGE_SIZE)
#define MIN_BLOCK_SIZE PAGE_SIZE  // Minimum block size is the size of one page

typedef struct frame {
    int index;
    int order;
    int status;
} frame_t;

frame_t frames[FRAME_COUNT];

void frames_init(){
    uart_int(FRAME_COUNT);
    uart_puts("\n");
    for(int i=0; i<FRAME_COUNT; i++){
        frames[i].index = i;
        frames[i].order = 0;
        frames[i].status = -1; // not allocated
    }
    merge_free(0);
}

void merge_free(int print){ // print is to show log
    int merged = 0;
    for(int i=0; i<FRAME_COUNT; i++){
        if(frames[i].status == -1){// not allocated and not a buddy
            int bud = i ^ (1 << frames[i].order);
            if(bud < FRAME_COUNT && bud >= 0){
                if(frames[bud].status == -1 && frames[bud].order == frames[i].order && frames[i].order < MAX_ORDER){
                    
                    int min_index = (i < bud) ? i : bud;
                    int max_index = (i > bud) ? i : bud;
                    frames[min_index].order += 1;
                    //frames[max_index].order += 1;
                    frames[max_index].status = 0; // a part of a large memory
                    if(print){
                        uart_puts("Merged index ");
                        uart_int(i);
                        uart_puts(" and ");
                        uart_int(bud);
                        uart_puts(" in order ");
                        uart_int(frames[i].order);
                        uart_puts("\n");
                        uart_send('\r');
                    }
                    merged = 1;
                }
            }
        }
    }
    if(merged)
        merge_free(print);
}

void free_page(unsigned long address){
    /* 
    1. find index of the memory
    2. set the memory and its buddy to be not allocated
    3. merge pages
    */
    
    int i = (address - MEMORY_START)/PAGE_SIZE;
    uart_int(i);
    uart_puts(" in freepage\n");
    if(frames[i].status == 1){
        frames[i].status = -1;
        merge_free(1);
    }
    else{
        uart_puts("invalid frame\n");
        uart_send('\r');
    }
}

int get_order(unsigned long size) {
    int order = 0;
    size = (size + PAGE_SIZE - 1) / PAGE_SIZE;  // Convert size to number of pages

    // Adjust size to the next power of two if it's not already
    size--;
    while (size > 0) {
        size >>= 1;
        order++;
    }
    return order;
}

void* allocate_page(unsigned long size) {
    int order = get_order(size);

    if (order > MAX_ORDER) {
        uart_puts("Requested size too large\n");
        return 0;
    }

    // Find the first free block with at least the required order
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (frames[i].status == -1 && frames[i].order >= order) {
            // Allocate this frame
            frames[i].status = 1;  // Mark as allocated

            // Split if necessary
            while (frames[i].order > order) {
                frames[i].order--;
                int buddy_index = i + (1 << frames[i].order);
                frames[buddy_index].order = frames[i].order;
                frames[buddy_index].status = -1;  // Buddy is free
            }

            uart_puts("Allocated at index ");
            uart_int(i);
            uart_puts(" of order ");
            uart_int(frames[i].order);
            uart_puts("\n");
            uart_send('\r');

            return (void*)(MEMORY_START + i * PAGE_SIZE);
        }
    }

    uart_puts("No suitable block found\n");
    return 0;
}

void print_frame_status(int len){
    for(int i=0;i<len;i++){
        uart_int(frames[i].status);
        uart_puts(" ");
    }
    uart_puts("\n");
    uart_send('\r');
}

void demo_page_alloc(){
    void * page0 = allocate_page(4000);
    print_frame_status(65);
    void * page1 = allocate_page(8000);
    print_frame_status(65);
    void * page2 = allocate_page(12000);
    print_frame_status(65);
    uart_hex(page0);
    uart_puts("\n");
    uart_send('\r');
    uart_hex(page1);
    uart_puts("\n");
    uart_send('\r');
    uart_hex(page2);
    uart_puts("\n");
    uart_send('\r');
    free_page(page0);
    print_frame_status(65);
    free_page(page1);
    print_frame_status(65);
    void * page3 = allocate_page(4096 * 2 * 2 * 2 * 2 + 1);
    print_frame_status(65);
}

int pool_sizes[] = {16, 32, 48, 96};
#define NUM_POOLS 4

typedef struct memory_pool {
    unsigned long start;   // Starting address of the pool
    int bitmap[PAGE_SIZE/16]; // Bitmap for free/allocated slots
    int slot_size;         // Size of each slot in bytes
    int total_slots;       // Total slots in the pool
} memory_pool_t;

memory_pool_t pools[NUM_POOLS];

void init_memory(){
    for(int i=0; i<NUM_POOLS; i++){
        pools[i].start = (unsigned long) allocate_page(PAGE_SIZE);
        pools[i].slot_size = pool_sizes[i];
        pools[i].total_slots = PAGE_SIZE/pool_sizes[i];
    }
}

void * malloc(unsigned long size){
    for(int i=0; i<NUM_POOLS; i++){
        if(pools[i].slot_size > size){
            // allocate a chunck
            for(int j = 0; j < pools[i].total_slots; j++){
                if(pools[i].bitmap[j] == 0){// the chunk is free
                    uart_puts("Memory allocated in chunck size ");
                    uart_int(pools[i].slot_size);
                    uart_puts(" in block ");
                    uart_int(j);
                    uart_puts(", physical address: ");
                    uart_hex(pools[i].start + j * pools[i].slot_size);
                    uart_puts("\n\r");
                    pools[i].bitmap[j] = 1;
                    return (void*)(pools[i].start + j * pools[i].slot_size);
                }
            }
            uart_puts("No more slots, should allocate new page");
            return 0;
        }
        else{
            continue;
        }
    }
    uart_puts("The input size is too large!\n");
    return 0;
}

void free(void* ptr) {
    for (int i = 0; i < NUM_POOLS; i++) {
        unsigned long address = (unsigned long) ptr;
        if (address >= pools[i].start && address < pools[i].start + PAGE_SIZE) {
            int slot = (address - pools[i].start) / pools[i].slot_size;
            pools[i].bitmap[i] = 0;
            uart_puts("Free memory with chunck size ");
            uart_int(pools[i].slot_size);
            uart_puts(" in block ");
            uart_int(slot);
            uart_puts(", physical address: ");
            uart_hex(address);
            uart_puts("\n\r");
            return;
        }
    }
}