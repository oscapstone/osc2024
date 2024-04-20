/*
OSC2024 LAB4
Author: jerryyyyy708
Starts from: 2024/04/19

frame list is a list to map index into page frame memory, 
there is only index in struct frame_t, the actual address can be calculated by (void*)(MEMORY_START + i * PAGE_SIZE)

On the other hand, memorypool_t places the actual address into start, and a bitmap to monitor the chunks are in use or not.
The index(slot) of memory can be lookup by first check which pool is the address in, then (address - pool -> start) / pool -> slot_size;
*/
#include "uart.h"

#define MAX_ORDER 5
#define PAGE_SIZE 4096  // Assuming a page size of 4KB
#define MEMORY_START 0x10000000
#define MEMORY_SIZE 0x10000000  // Managing 256MB of memory
#define FRAME_COUNT (MEMORY_SIZE / PAGE_SIZE)
#define MIN_BLOCK_SIZE PAGE_SIZE  // Minimum block size is the size of one page

typedef struct frame {
    //save order and status for convinence to maintain, use convert_val_and_print() to convert to the actual entry var.
    int index;
    int order;
    int status;
    struct frame *next;
    struct frame *prev;
} frame_t;

frame_t frames[FRAME_COUNT];

/*
1. during init, complete all merge and then place all into freelist (or simply put all into freelist and merge) V
2. allocate handle: if there is cut frame, put cut frames into freelist, then move itself out of freelist
3. free handle: put idx back to free list, and merge (handle by merge)
4. merge handle: move itself to another order and buddy out of free list V
*/

typedef struct freelist {
    frame_t * head;
} freelist_t;

freelist_t fl[MAX_ORDER + 1];


void status_instruction(){
    uart_puts("Buddy system value definition\n\r");
    uart_puts("val >= 0: allocable contiguous memory starts from the idx with order val\n\r");
    uart_puts("val = -1: free but belongs to a larger contiguous memory block\n\r");
    uart_puts("val = -2: allocated block\n\r");
}

void print_freelist(int head){
    uart_puts("Printing free lists:\n");
    for(int i = 0; i <= MAX_ORDER; i++){
        uart_puts("Order ");
        uart_int(i);
        uart_puts(": ");
        frame_t *temp = fl[i].head;
        if (!temp) uart_puts("empty\n");
        int count = 0;
        while(temp){
            if(count == head && head != 0)
                break;
            uart_int(temp->index);
            uart_puts(" -> ");
            temp = temp->next;
            count++;
        }

        if(head != 0)
            uart_puts("... \n\r");
        else
            uart_puts("NULL\n\r");
    }
}


void frames_init(){
    status_instruction();
    uart_int(FRAME_COUNT);
    uart_puts("\n");
    for(int i=0; i<FRAME_COUNT; i++){
        frames[i].index = i;
        frames[i].order = 0;
        frames[i].status = -1; // not allocated
        frames[i].next = 0;
    }
    fl[0].head = frames;
    frame_t * temp = fl[0].head;
    for(int i=1; i<FRAME_COUNT; i++){
        temp -> next = &frames[i];
        temp -> next -> prev = temp;
        temp = temp -> next;
    }
    merge_free(0);
    print_freelist(5);
}

void remove_from_freelist(frame_t *frame) {
    if (frame->prev) {
        frame->prev->next = frame->next;
    }
    if (frame->next) {
        frame->next->prev = frame->prev;
    }
    if (fl[frame->order].head == frame) {
        fl[frame->order].head = frame->next;
    }
    frame->next = frame->prev = 0;
}

void insert_into_freelist(frame_t *frame, int order) {
    frame->next = fl[order].head;
    if (fl[order].head) {
        fl[order].head->prev = frame;
    }
    frame->prev = 0;
    fl[order].head = frame;
}

void merge_free(int print) {
    int merged = 0;
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (frames[i].status == -1 && frames[i].order < MAX_ORDER) {  // Check only non-allocated frames
            int bud = i ^ (1 << frames[i].order);  // Calculate buddy index
            if (bud < FRAME_COUNT && frames[bud].status == -1 && frames[bud].order == frames[i].order) {
                int min_index = (i < bud) ? i : bud;
                int max_index = (i > bud) ? i : bud;

                // Remove both frames from their current free list
                remove_from_freelist(&frames[min_index]);
                remove_from_freelist(&frames[max_index]);

                // Update the order of the minimum index frame
                frames[min_index].order++;
                frames[max_index].status = 0;  // Mark the max index frame as part of a larger block
                
                // Insert the merged frame into the next order's free list
                insert_into_freelist(&frames[min_index], frames[min_index].order);

                if (print) {
                    uart_puts("Merged index ");
                    uart_int(min_index);
                    uart_puts(" and ");
                    uart_int(max_index);
                    uart_puts(" into order ");
                    uart_int(frames[min_index].order);
                    uart_puts("\n");
                    uart_send('\r');
                }
                merged = 1;  // Flag that a merge occurred
                //break;  // Break to restart from the beginning due to list modification
            }
        }
    }
    if (merged) {
        merge_free(print);  // Only recurse if a merge occurred
    }
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
            if(pool -> total_slots != PAGE_SIZE/pool -> slot_size) //not the first page, the header is also placed in page frame
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