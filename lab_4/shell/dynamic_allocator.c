#include "include/dynamic_allocator.h"
#include "include/page_allocator.h"

// #define POOL {4, 8, 16, 32, 64, 128, 256, 512, 1024}
// #define PARTITION {4, 2, 2, 2, 2, 2, 2, 2, 2}
// #define POOL_SIZE 8
// // max pages allowed to be partitioned
// #define MAX_PAGE 64

// typedef struct Page{
//     void* prefix;
//     // the occupied section of each partition size in page
//     int usage[POOL_SIZE];
// }

// typedef struct DynamicAllocator{
//     int chunk_option[POOL_SIZE];
//     int partition[POOL_SIZE];
//     int allocated_page_num;
//     Page allocated_page_info[MAX_PAGE];

// }DynamicAllocator;


struct DynamicAllocator dynamic_allocator;


void init_dynamic_allocator(){
    int pool[] = POOL;
    int part[] = PARTITION;
    dynamic_allocator.allocated_page_num = 0;
    for(int i=0;i<POOL_SIZE;i++){
        dynamic_allocator.chunk_option[i] = pool[i];
        dynamic_allocator.partition[i] = part[i];
    }
    for(int i=0;i<MAX_PAGE;i++){
        dynamic_allocator.allocated_page_info[i].prefix = NULL;
        for (int j=0;j<POOL_SIZE;j++){
            dynamic_allocator.allocated_page_info[i].usage[j] = 0;
        }
    }
}

void* malloc(unsigned long size){
    // round up to cloest size
    int pool[] = POOL;
    int part[] = PARTITION;
    int index=-1; // this pool index that this malloc size should get
    for(int i=0;i<POOL_SIZE;i++){
        if(size<pool[i]){
            index=i;
            break;
        }
    }
    // size too big, allocate pages directly
    if(index == -1){
        uart_hex(size/PAGE_SIZE);
        return (void*)malloc_page((size/PAGE_SIZE)+1);
    }
    // check if there exist the available pool
    for(int i=0;i<dynamic_allocator.allocated_page_num;i++){
        if(dynamic_allocator.allocated_page_info[i].usage[index]>0){
            void* address = dynamic_allocator.allocated_page_info[i].prefix;
            unsigned int used_offset = 0x00;
            for(int j=0;j<index;j++){
                // shift the offset
                used_offset = used_offset+(part[j]*pool[j]);
            }
            if(pool[index] != dynamic_allocator.allocated_page_info[i].usage[index])
                used_offset = used_offset+ (pool[index]-dynamic_allocator.allocated_page_info[i].usage[index])+1;
            address = address+used_offset;
            dynamic_allocator.allocated_page_info[i].usage[index]--;
            uart_puts("[Dynamic Malloc] Malloc size ");
            uart_hex(pool[index]);
            uart_puts(" started from address ");
            uart_hex(address);
            uart_puts(" has been malloced.\n");
            return address;
        }
    }
    // non available pool section found, allocate a new page for dy_allocator
    void* address = (void*)malloc_page(1);
    // init the usage of each partition
    dynamic_allocator.allocated_page_info[dynamic_allocator.allocated_page_num].prefix = (unsigned int)address & 0xFFFFF000;
    for(int i=0;i<POOL_SIZE;i++){
        dynamic_allocator.allocated_page_info[dynamic_allocator.allocated_page_num].usage[i] = part[i];
    }
    unsigned int used_offset = 0x00;
    for(int j=0;j<index;j++){
        // shift the offset
        used_offset = used_offset+(part[j]*pool[j]);
    }
    address = address+used_offset;
    dynamic_allocator.allocated_page_info[dynamic_allocator.allocated_page_num].usage[index]--;
    uart_puts("[Dynamic Malloc] Malloc size ");
    uart_hex(pool[index]);
    uart_puts(" started from address ");
    uart_hex(address);
    uart_puts(" has been malloced.\n");
    dynamic_allocator.allocated_page_num++;
    return address;
}