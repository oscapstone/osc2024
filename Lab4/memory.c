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
#include "dtb.h"

extern char _end; //the end if kernel

#define MAX_ORDER 6
#define PAGE_SIZE 4096  // Assuming a page size of 4KB
//#define MEMORY_START 0x00 //0x10000000
//#define MEMORY_SIZE 0x3C000000 //simply hardcode, get 0x3B400000 in the device tree
//#define FRAME_COUNT (MEMORY_SIZE / PAGE_SIZE)

typedef struct frame {
    //save order and status for convinence to maintain, use convert_val_and_print() to convert to the actual entry var.
    int index; //just lazy to calculate
    int order;
    int status; //-1: free, 0: part of a block, 1: allocated
    struct frame *next;
    struct frame *prev;
} frame_t;

frame_t *frames;//[FRAME_COUNT];
int frame_count;

//startup allocation
/*
Allocate the buddy system after _end, 
we need to return allocated mem and also get the current place,
so use void ** to keep track of the pointer (for memory reserve)
*/
void *simple_alloc(void **cur, int size) {
    void *allocated = *cur;
    *cur = (char *)(*cur) + size; //convert for operator calculation
    //memset 0
    char *p = allocated;
    for (int i = 0; i < size; i++) {
        p[i] = 0;
    }
    return allocated;
}

/*
1. during init, complete all merge and then place all into freelist (or simply put all into freelist and merge) V
2. allocate handle: if there is cut frame, put cut frames into freelist, then move itself out of freelist V
3. free handle: put idx back to free list, and merge (handle by merge) V
4. merge handle: move itself to another order and buddy out of free list V
*/

typedef struct freelist {
    frame_t * head;
} freelist_t;

freelist_t * fl; //[MAX_ORDER + 1]; //there are MAX_ORDER+1 freelists

//moemory pools, can manually set more
int pool_sizes[] = {16, 32, 48, 96};
#define NUM_POOLS 4

typedef struct memory_pool {
    unsigned long start;   // Starting address of the pool
    int bitmap[PAGE_SIZE/16]; // Bitmap for free 0/allocated 1 slots
    int slot_size;         // Size of each slot in bytes
    int total_slots;       // Total slots in the pool
    struct memory_pool *next; // next page with the same size(will be set after cuurent page is full)
} memory_pool_t;

memory_pool_t *pools; //[NUM_POOLS];

void remove_from_freelist(frame_t *frame) {
    // previous' next to current's next
    if (frame->prev) {
        frame->prev->next = frame->next;
    }

    // next's prev to current's prev
    if (frame->next) {
        frame->next->prev = frame->prev;
    }

    // if current is head, set head to next
    if (fl[frame->order].head == frame) {
        fl[frame->order].head = frame->next;
    }

    //remove current
    frame->next = frame->prev = 0;
}

void insert_into_freelist(frame_t *frame, int order) {
    //put current into the head of freelist
    frame->next = fl[order].head;

    if (fl[order].head) {// head is not NULL, set prev
        fl[order].head->prev = frame;
    }

    frame->prev = 0;
    fl[order].head = frame;
}


void status_instruction(){
    uart_puts("Initializing buddy system, value definition\n\r");
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
        int count = 0;
        while(temp){
            if(count == head && head != 0)
                break;
            uart_int(temp->index);
            uart_puts(" -> ");
            temp = temp->next;
            count++;
        }

        if(temp)
            uart_puts("... \n\r");
        else
            uart_puts("NULL\n\r");
    }
}

/*
Reserve memories:
1. Spin tables for multicore boot (0x0000 - 0x1000) -> hardcode V
2. Kernel image in the physical memory -> address of _end V
3. Initramfs -> use dtb V
4. Devicetree (Optional, if you have implement it) -> dtb_base + totalsize V
5. Your simple allocator (startup allocator) -> in kernel V
*/

void memory_reserve(unsigned long start, unsigned long end) {
    // get start and end index
    unsigned long start_index = (start - memory_start) / PAGE_SIZE;
    unsigned long end_index = (end - memory_start) / PAGE_SIZE;

    // make sure end is covered
    if((end - memory_start) % PAGE_SIZE != 0)
        end_index++;

    for (unsigned long i = start_index; i <= end_index && i < frame_count; i++) {
        if (frames[i].status == -1) {
            frames[i].status = 1;
            remove_from_freelist(&frames[i]);
        }
        else{ // already in used
            uart_puts("Warning, there might be a bug!");
        }
    }

    uart_puts("Reserved from frame ");
    uart_int(start_index);
    uart_puts(" to frame ");
    uart_int(end_index);
    uart_puts(".\n\r");
}

void frames_init(){
    // get the start and end address of useable memory
    get_memory(dtb_start);

    split_line();
    frame_count = (memory_end - memory_start) / PAGE_SIZE;
    status_instruction();

    // startup allocation, allocate after the kernel
    void * base = (void *) &_end;
    // sending &base will keep track of the memory required to reserve
    
    //frame array
    frames = simple_alloc(&base ,(int) sizeof(frame_t) * frame_count);
    //freelist
    fl = simple_alloc(&base, (int) sizeof(freelist_t) * (MAX_ORDER + 1));
    //memory pool
    pools = simple_alloc(&base, (int) sizeof(memory_pool_t) * NUM_POOLS);
    split_line();
    uart_getc();

    uart_int(frame_count);
    uart_puts("\n\r");
    split_line();
    // init all frame with order 0 free
    for(int i=0; i<frame_count; i++){
        frames[i].index = i;
        frames[i].order = 0;
        frames[i].status = -1; // not allocated
        frames[i].next = 0;
        frames[i].prev = 0;
    }
    fl[0].head = frames;
    frame_t * temp = fl[0].head;
    //place all frame into free list (when init, small is in front)
    for(int i=1; i<frame_count; i++){
        temp -> next = &frames[i];
        temp -> next -> prev = temp;
        temp = temp -> next;
    }
    // handle reserve here, remove reserved from freelist and set status to allocated
    // memory_reserve(0x0000, 0x1000);
    // memory_reserve(0x80000, base);
    // spin table, stack, kernel (and simple allocator), frame array, initramfs, dtb 
    memory_reserve(0x0000, base); //to save the stack from being allocated
    memory_reserve(cpio_base, cpio_end);
    memory_reserve(dtb_start, dtb_end);
    split_line();
    uart_getc();
    //After init all frame, merge them
    merge_all(0);
    print_freelist(3);
    split_line();
}

void merge_free(frame_t *frame, int print){
    // find buddy and check if the buddy is able to merge
    int index = frame->index;
    int order = frame->order;
    int buddy_index = index ^ (1 << order);

    // check buddy status
    if (buddy_index < frame_count && frames[buddy_index].status == -1 && frames[buddy_index].order == order && order < MAX_ORDER) {
        // start merging
        remove_from_freelist(frame);
        remove_from_freelist(&frames[buddy_index]);
        
        //set smaller index to free and larger to part of large memory 
        int min_index = (index < buddy_index) ? index : buddy_index;
        int max_index = (index > buddy_index) ? index : buddy_index;

        frames[min_index].order++;
        frames[min_index].status = -1; // free
        frames[max_index].status = 0; // part

        insert_into_freelist(&frames[min_index], frames[min_index].order);

        if (print) {
            uart_puts("Merged index ");
            uart_int(index);
            uart_puts(" and ");
            uart_int(buddy_index);
            uart_puts(" into order ");
            uart_int(frames[min_index].order);
            uart_puts("\n");
            uart_send('\r');
        }

        merge_free(&frames[min_index], print);
    }
}

void merge_all(int print){
    int merged = 0;
    //tranverse through all frame and check is it able to merge
    for (int i = 0; i < frame_count; i++) {
        if (frames[i].status == -1 && frames[i].order < MAX_ORDER) {  // Check only non-allocated frames
            int bud = i ^ (1 << frames[i].order);  // Calculate buddy index
            if (bud < frame_count && frames[bud].status == -1 && frames[bud].order == frames[i].order) {
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
                merged = 1; 
            }
        }
    }
    if (merged) {
        merge_all(print);
    }
}


void free_page(unsigned long address){
    /* 
    1. find index of the memory
    2. set the memory and its buddy to be not allocated
    3. merge pages
    */
    
    int i = (address - memory_start)/PAGE_SIZE;
    uart_int(i);
    uart_puts(" in freepage\n");
    if(frames[i].status == 1){//allocated
        frames[i].status = -1;
        insert_into_freelist(&frames[i], frames[i].order);
        //merge_all(1);
        merge_free(&frames[i], 1);
        //print_freelist(3);
    }
    else{
        uart_puts("invalid frame\n");
        uart_send('\r');
    }
}

int get_order(unsigned long size){
    int order = 0;
    //upbounding, 4096 -> 1, 4097 -> 2, see need hom many page
    size = (size + PAGE_SIZE - 1) / PAGE_SIZE; 

    //make sure if size is 2^? will be correct
    size--;
    while (size > 0) {
        size >>= 1;
        order++;
    }
    return order;
}

void* allocate_page(unsigned long size){
    int order = get_order(size);
    if (order > MAX_ORDER) {
        uart_puts("Requested size too large\n");
        return 0;
    }

    // Check free list from the requested order
    for (int current_order = order; current_order <= MAX_ORDER; current_order++) {
        frame_t *frame = fl[current_order].head;
        if (frame != 0) { // Found a free frame at this order
            // Remove the frame from the free list
            remove_from_freelist(frame);
            frame->status = 1; //allocated

            // Split the frame if its order is higher than needed
            while (frame->order > order) {
                uart_puts("Split frame from order ");
                uart_int(frame -> order);
                frame->order--;
                uart_puts(" to ");
                uart_int(frame -> order);
                uart_puts("\n\rFrame index: ");
                uart_int(frame -> index);
                uart_puts(" and ");
                int buddy_index = frame->index + (1 << frame->order);
                frames[buddy_index].order = frame->order;
                frames[buddy_index].status = -1; // Buddy is now free
                uart_int(buddy_index);
                uart_puts("\n\r");
                insert_into_freelist(&frames[buddy_index], frame->order); // Add buddy to the free list
                //merge_free(&frames[buddy_index], 1);
            }

            uart_puts("Allocated at index ");
            uart_int(frame->index);
            uart_puts(" of order ");
            uart_int(frame->order);
            uart_puts("\n");
            uart_send('\r');
            return (void*)(memory_start + frame->index * PAGE_SIZE);
        }
    }
    
    uart_puts("No suitable block found\n");
    return 0;
}


void convert_val_and_print(int start, int len){//convert into val
    int prev = 99;
    uart_puts("Frame status from ");
    uart_int(start);
    uart_puts(": ");
    for(int i=start;i<start+len;i++){
        if(frames[i].status == 1){ //allocated for sure, -2
            uart_int(-2);
            prev = -2;
            uart_puts(" ");
        }
        else if(frames[i].status == 0){// might be free but belongs or allocated
            //in my implementation, status 0 means it's part of a block, no matter allocated or not
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

void print_status(int len){
    for(int i=0;i<len;i++){
        uart_int(frames[i].status);
        uart_puts(" ");
    }
    uart_puts("\n");
    uart_send('\r');
}

void allocate_all(){
    //helper function for demo, allocate all frame with order smaller than max_order
    int times = 1;
    int base = 4096;
    void * temp;
    for(int i=0; i<MAX_ORDER; i++){
        frame_t * cur = fl[i].head;
        while(cur){
            cur = cur -> next;
            temp = allocate_page(base * times);
        }
        times *= 2;
    }
}

void demo_page_alloc(){
    split_line();
    uart_puts("Allocate all page with order lower than 6.\n\r");
    allocate_all();
    split_line();
    uart_puts("Now there is only free page with order 6\n\r");
    uart_getc();
    print_freelist(3);
    convert_val_and_print(frame_count - 64, 64);
    split_line();
    uart_puts("Show releasing redundant memory block by allocating order 0 page\n\r");
    uart_getc();
    void * page10 = allocate_page(4000);
    uart_getc();
    split_line();
    print_freelist(3);
    convert_val_and_print(frame_count - 64, 64);
    // split_line();
    // uart_getc();
    // print_freelist(3);
    // convert_val_and_print(frame_count - 64, 64);
    split_line();
    uart_puts("Page Allocated, (next: free page)!\n\r");
    uart_puts("Free the order 0 page to show merging iteratively\n\r");
    uart_getc();
    free_page(page10);
    split_line();
    uart_getc();
    print_freelist(3);
    convert_val_and_print(frame_count - 64, 64);
    split_line();
}

void init_memory(){
    // allocate a page for each memory pool, startup clears byte to 0 -> safe
    for(int i=0; i<NUM_POOLS; i++){
        pools[i].start = (unsigned long) allocate_page(PAGE_SIZE);
        pools[i].slot_size = pool_sizes[i];
        pools[i].total_slots = PAGE_SIZE/pool_sizes[i];
        pools[i].next = 0;
    }
}

void * malloc(unsigned long size){
    for(int i=0; i<NUM_POOLS; i++){
        if(pools[i].slot_size >= size){
            // allocate a chunck from the pool
            memory_pool_t * pool = &pools[i];
            while(1){
                //go through all slot in current pool
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
                // if not allocated -> see next page if there is one
                if(pool -> next)
                    pool = pool -> next;
                else
                    break;
            }
            //no more free memory in pool
            uart_puts("No more slots, start allocating new page\n");
            //allocate a new page for memory pool
            memory_pool_t *new_pool = (memory_pool_t *) allocate_page(sizeof(memory_pool_t));
            
            //header is also in the pool, so start should add sizeof header
            new_pool -> start = new_pool + sizeof(memory_pool_t);
            new_pool -> slot_size = pool -> slot_size;
            //init the bitmap to 0
            for(int k = 0; k < (PAGE_SIZE/16); k++){
                new_pool -> bitmap[k] = 0;
            }
            //get num of the slots
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

void free(void* ptr){
    /*
    Iterate through all memory pool (and their subpools) and see is the memory inside it,
    if so, free the specific chunck by calculate index of it
    */
    for (int i = 0; i < NUM_POOLS; i++) {
        memory_pool_t * pool = &pools[i];
        while(pool){
            //get end address of the pool by start + offset
            unsigned long offset = PAGE_SIZE;
            //for first pool, can allocate whole page, for others, need to - header
            //check is it the first by calculating total slot
            if(pool -> total_slots != PAGE_SIZE/pool -> slot_size) //not the first page, the header is also placed in page frame
                offset -= sizeof(memory_pool_t);
            unsigned long address = (unsigned long) ptr;
            if (address >= pool -> start && address < pool -> start + offset) {
                //get slot bit
                int slot = (address - pool -> start) / pool -> slot_size;
                //set to free
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