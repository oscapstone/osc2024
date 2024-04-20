/*
OSC2024 LAB4
Author: jerryyyyy708
Starts from: 2024/04/19
*/
#include "uart.h"

#define MAX_ORDER 5
#define PAGE_SIZE 4096  // Assuming a page size of 4KB
#define MEMORY_START 0x10000000
#define MEMORY_SIZE 0x10000000  // Managing 256MB of memory
#define FRAME_COUNT (MEMORY_SIZE / PAGE_SIZE)
#define MIN_BLOCK_SIZE PAGE_SIZE  // Minimum block size is the size of one page

typedef struct frame {
    /* 
    save order and status for convinence to maintain, 
    use convert_val_and_print to convert to the actual entry var.
    */
    int index;
    int order;
    int status;
} frame_t;

frame_t frames[FRAME_COUNT];

void status_instruction(){
    uart_puts("Buddy system value definition\n\r");
    uart_puts("val >= 0: allocable contiguous memory starts from the idx with order val\n\r");
    uart_puts("val = -1: free but belongs to a larger contiguous memory block\n\r");
    uart_puts("val = -2: allocated block\n\r");
}

void frames_init(){
    status_instruction();
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
                    frames[max_index].order = -1;
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

void convert_val_and_print(int len){//convert into val
    int prev = 99;
    for(int i=0;i<len;i++){
        if(frames[i].status == 1){ //allocated for sure, -2
            uart_int(-2);
            prev = -2;
            uart_puts(" ");
        }
        else if(frames[i].status == 0){// might be free but belongs or allocated
            if(prev >= 0 || prev != -2){ // free but belongs
                prev = -1;
                uart_int(-1);
                uart_puts(" ");
            }
            else{ // allocated
                uart_int(-2);
                prev = -2;
                uart_puts(" ");
            }
        }
        else{
            uart_int(frames[i].order); // free for sure 
            uart_puts(" ");
            prev = frames[i].order;
        }
    }
    uart_puts("\n");
    uart_send('\r');
}

/*
void convert_val_and_print(int len){
    for(int i=0;i<len;i++){
        uart_int(frames[i].status);
        uart_puts(" ");
    }
    uart_puts("\n");
    uart_send('\r');
}
*/

void demo_page_alloc(){
    void * page0 = allocate_page(4000);
    convert_val_and_print(65);
    void * page1 = allocate_page(8000);
    convert_val_and_print(65);
    void * page2 = allocate_page(12000);
    convert_val_and_print(65);
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
    convert_val_and_print(65);
    free_page(page1);
    convert_val_and_print(65);
    void * page3 = allocate_page(4096 * 2 * 2 * 2 * 2 + 1);
    convert_val_and_print(65);
}

int pool_sizes[] = {16, 32, 48, 96};
#define NUM_POOLS 4

typedef struct memory_pool {
    unsigned long start;   // Starting address of the pool
    int bitmap[PAGE_SIZE/16]; // Bitmap for free/allocated slots
    int slot_size;         // Size of each slot in bytes
    int total_slots;       // Total slots in the pool
    struct memory_pool *next;
} memory_pool_t;

memory_pool_t pools[NUM_POOLS];

void init_memory(){
    for(int i=0; i<NUM_POOLS; i++){
        pools[i].start = (unsigned long) allocate_page(PAGE_SIZE);
        pools[i].slot_size = pool_sizes[i];
        pools[i].total_slots = PAGE_SIZE/pool_sizes[i];
        pools[i].next = 0;
    }
}

void * malloc(unsigned long size){
    for(int i=0; i<NUM_POOLS; i++){
        if(pools[i].slot_size > size){
            // allocate a chunck
            memory_pool_t * pool = &pools[i];
            while(1){
                for(int j = 0; j < pool -> total_slots; j++){
                    if(pool -> bitmap[j] == 0){// the chunk is free
                        uart_puts("Memory allocated in chunck size ");
                        uart_int(pool -> slot_size);
                        uart_puts(" in block ");
                        uart_int(j);
                        uart_puts(", physical address: ");
                        uart_hex(pool -> start + j * pool -> slot_size);
                        uart_puts("\n\r");
                        pool -> bitmap[j] = 1;
                        return (void*)(pool -> start + j * pool -> slot_size);
                    }
                }
                if(pool -> next)
                    pool = pool -> next;
                else
                    break;
            }
            uart_puts("No more slots, start allocating new page\n");
            //start allocate
            memory_pool_t *new_pool = (memory_pool_t *) allocate_page(sizeof(memory_pool_t));
            new_pool -> start = new_pool + sizeof(memory_pool_t);
            new_pool -> slot_size = pool -> slot_size;
            for(int k =0; k < (PAGE_SIZE/16); k++){
                new_pool -> bitmap[k] = 0;
            }
            new_pool -> total_slots = (PAGE_SIZE - sizeof(memory_pool_t)) / new_pool -> slot_size;
            new_pool -> next = 0;
            pool -> next = new_pool;
            return malloc(size); // dangerous, can try to modify
        }
        else{
            continue;
        }
    }
    uart_puts("The input size is too large for malloc, assign page instead!\n\r");
    return allocate_page(size);
}

void free(void* ptr) {
    for (int i = 0; i < NUM_POOLS; i++) {
        memory_pool_t * pool = &pools[i];
        while(pool){
            unsigned long offset = PAGE_SIZE;
            if(pool -> total_slots != PAGE_SIZE/pool -> slot_size) //not the first page
                offset -= sizeof(memory_pool_t);
            unsigned long address = (unsigned long) ptr;
            if (address >= pool -> start && address < pool -> start + offset) {
                int slot = (address - pool -> start) / pool -> slot_size;
                pool -> bitmap[slot] = 0;
                uart_puts("Free memory with chunck size ");
                uart_int(pool -> slot_size);
                uart_puts(" in block ");
                uart_int(slot);
                uart_puts(", physical address: ");
                uart_hex(address);
                uart_puts("\n\r");
                return;
            }
            pool = pool -> next;
        }
    }
    uart_puts("Invalid address!\n\r");
}