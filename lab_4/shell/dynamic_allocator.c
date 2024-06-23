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
    for(int i=0;i<MAX_PAGE;i++){
        dynamic_allocator.allocated_page_info[i].prefix = NULL;
        for (int j=0;j<POOL_SIZE;j++){
            for(int k=0;k<4;k++){
                dynamic_allocator.allocated_page_info[i].slot[j][k] = 0; 
            }
        }
    }
}

int free(void* to_free){
    int pool[] = POOL;
    int part[] = PARTITION;
    uart_puts("address 0x");
    uart_hex(to_free);
    uart_puts("\n");
    unsigned int* prefix_address = ((unsigned int)to_free & (unsigned int)0xFFFFF000);
    unsigned int* offset = ((unsigned int)to_free & (unsigned int)0x00000FFF);
    for(int i=0;i<dynamic_allocator.allocated_page_num;i++){
        // find the corresponding page by prefix
        if((unsigned int*)dynamic_allocator.allocated_page_info[i].prefix == prefix_address){
            // find the size that freer used by offset
            for(int k=0; k<POOL_SIZE;k++){
                for(int j=0;j<part[k];j++){
                    if(offset == 0){
                        dynamic_allocator.allocated_page_info[i].slot[k][j] = 0;
                        uart_puts("space size 0x");
                        uart_hex(pool[k]);
                        uart_puts(" is freed\n");
                        return 0;
                    }
                    else if(offset < pool[k]){
                        // assert error
                        uart_hex(offset);
                        uart_hex(pool[j]);
                        uart_puts("[Assert Error] offset error\n");
                        return 1;
                    }
                    else{
                        offset = (void*)offset -(void*)pool[k];

                    }
                }
            }
            //dynamic_allocator.allocated_page_info[i];
        }
    }
    free_page(to_free);
    // uart_puts("[Assert Error] Page to be freed does not exist\n");
    return 1;
}

void* malloc(unsigned long size){
    // round up to cloest size
    int pool[] = POOL;
    int part[] = PARTITION;
    int index=-1; // this pool index that this malloc size should get
    for(int i=0;i<=POOL_SIZE;i++){
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
        int j=0;
        for(j=0;j<part[index];j++){
            if(dynamic_allocator.allocated_page_info[i].slot[index][j] == 0){
                void* address = dynamic_allocator.allocated_page_info[i].prefix;
                unsigned int used_offset = 0x00;
                for(int k=0;k<index;k++){
                    // shift the offset
                    used_offset = used_offset+(part[k]*pool[k]);
                }
                if(j!=0)
                    used_offset = used_offset+ (j*pool[index]);
                address = address+used_offset;
                dynamic_allocator.allocated_page_info[i].slot[index][j]=1;
                uart_puts("[Dynamic Malloc] Malloc size 0x");
                uart_hex(pool[index]);
                uart_puts(" started from address 0x");
                uart_hex(address);
                uart_puts(" has been malloced.\n");
                return address;
            }
        }
    }
    // non available pool section found, allocate a new page for dy_allocator
    void* address = (void*)malloc_page(1);
    // init the usage of each partition
    dynamic_allocator.allocated_page_info[dynamic_allocator.allocated_page_num].prefix = (unsigned int)address & 0xFFFFF000;
    for(int i=0;i<POOL_SIZE;i++){
        for(int j=0;j<4;j++){
            dynamic_allocator.allocated_page_info[dynamic_allocator.allocated_page_num].slot[i][j] = 0;
        }
    }
    unsigned int used_offset = 0x00;
    for(int j=0;j<index;j++){
        // shift the offset
        used_offset = used_offset+(part[j]*pool[j]);
    }
    address = address+used_offset;
    dynamic_allocator.allocated_page_info[dynamic_allocator.allocated_page_num].slot[index][0] = 1;
    uart_puts("[Dynamic Malloc] Malloc size 0x");
    uart_hex(pool[index]);
    uart_puts(" started from address 0x");
    uart_hex(address);
    uart_puts(" has been malloced.\n");
    dynamic_allocator.allocated_page_num++;
    return address;
}