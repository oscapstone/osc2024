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
                    }
                    merged = 1;
                }
            }
        }
    }
    if(merged)
        merge_free(print);
}

void free_page(void * address){//give a memory
    /* 
    1. find index of the memory
    2. set the memory and its buddy to be not allocated
    3. merge pages
    */
    int i = (address - MEMORY_START)/MEMORY_SIZE;
    if(frames[i] == 1){
        frames[i].status = -1;
        merge_free(1);
    }
    else
        uart_puts("invalid frame\n");
    
}

void* allocate_page(int size){

}