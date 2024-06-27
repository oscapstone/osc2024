#include "../include/allocator.h"

char *allocated = (char*)&__end;

buddy_system_t *buddy_system;
dynamic_memory_allocator_t *dma;

int small_block_pool[7] = {16, 32, 64, 128, 256, 512, 1024};

void buddy_init(void){
    buddy_system = (buddy_system_t *)BUDDY_METADATA_ADDR;
    buddy_system->buddy_list = (buddy_block_list_t**)(BUDDY_METADATA_ADDR + sizeof(buddy_system_t));
    buddy_block_list_t *list = (buddy_block_list_t *)(buddy_system->buddy_list);
    buddy_system->buddy_start_address = (void *)(0x0);
    buddy_system->buddy_end_address = (void *)(0x3C000000);
    buddy_system->max_block_amount = (1 << (MAX_ORDER - 1));
    buddy_system->max_index = buddy_system->max_block_amount-1;

    for(int i = 0; i < MAX_ORDER; i++){
        
        int alloc_offset = 0;
        int block_amount = (1 << (MAX_ORDER - i - 1));
        
        buddy_system->first_avail[i] = -1;
        buddy_system->list_addr[i] = (void*)list;

        buddy_block_list_t *cur = list;
        buddy_block_list_t *prev = NULL;

        for(int j = 0; j < block_amount; j++){
            cur->idx = (j * (1 << i));
            // all block are belonged to the largest continuous memory block 
            cur->order = STATUS_F;
            if(i == MAX_ORDER - 1){
                cur->order = MAX_ORDER - 1;
            }
            if(cur!=buddy_system->list_addr[0]){
                prev->next = cur;
            }

            cur->next = 0;
            cur->addr = buddy_system->buddy_start_address + alloc_offset;
            cur->size = PAGE_SIZE * (1 << i);
            // update the offset of buddy system memory
            alloc_offset += cur->size;
            prev = cur;
            // get next block of block list
            cur++;
        }
        // goto next block list
        list += block_amount;
    }
    // In the begining, only the largest block is available
    buddy_system->first_avail[MAX_ORDER - 1] = 0;
}

buddy_block_list_t *get_block_addr(int order,int index){
    //buddy_system->list_addr[order] can get he base address of the list containing blocks of same order 
    //buddy_system->first_avail[order] / (1 << order)) translate the global index into a local index within the array of blocks of a specific order.
    
    //don't known order (need more research)
    if(order==-1){
        buddy_block_list_t *cur = get_block_addr(0,index);
        buddy_block_list_t *last = cur;
        for(int i = 0; i < MAX_ORDER; i++){
            cur = get_block_addr(i,index);
            // index is illegal in this order, indicate that it must belonged to lower levels
            if(index % (1 << i) != 0 | cur->order != STATUS_X)
                return last;

            last = cur;
        }

        return cur;
    }
    else{
        //get first avialible block
        if(index==-1){
        return (buddy_block_list_t *)((void*)buddy_system->list_addr[order] + (buddy_system->first_avail[order] / (1 << order)) * sizeof(buddy_block_list_t));
        }
        else{
            return (buddy_block_list_t *)((void*)buddy_system->list_addr[order] + (index / (1 << order)) * sizeof(buddy_block_list_t));
        }
    }
    

    
}

void update_first_avail_list(void){
    int order,block_index;
    for(order = 0; order < MAX_ORDER; order++){
        buddy_block_list_t *cur = (buddy_block_list_t *)buddy_system->list_addr[order];
        for(block_index = 0; block_index < buddy_system->max_block_amount ; block_index += (1 << order)){

            if(cur->order >= 0)
                break;
            cur = cur->next;
        }
        if(block_index<buddy_system->max_block_amount){
            buddy_system->first_avail[order] = block_index;
        }
        else{
            buddy_system->first_avail[order] = -1;
        }
        //uart_int(buddy_system->first_avail[order]);
        //uart_send(' ');
    }
}

void* buddy_malloc(unsigned int size){
    int i;

    if(size > PAGE_SIZE * (1 << (MAX_ORDER - 1))){
        uart_puts("Error : Requested size is larger than availible space\n");
        return NULL;
    }

    for(i = 0; i < MAX_ORDER; i++){
        //find the big enough block
        if(PAGE_SIZE * (1 << i) >= size){
            if(buddy_system->first_avail[i] >= 0){

                buddy_block_list_t *cur = get_block_addr(i,-1);
                
                // first assuming that there's smaller block in lower layer
                cur->order = STATUS_F;

                int avail_index = buddy_system->first_avail[i];

                //split the block as small as possible
                buddy_block_list_t *buddy_allocated = buddy_split(avail_index, avail_index + (1 << i), size, i);
                mark_allocated(buddy_allocated);
                update_first_avail_list();

                uart_puts("Allocated block of order ");
                uart_int(log(buddy_allocated->size/PAGE_SIZE,2));
                uart_puts(" at: ");
                uart_hex_64((unsigned long long)buddy_allocated->addr);
                uart_puts("\n");
                return (void*)(buddy_allocated->addr);
            }
            else{
                continue;
            }
        }
    }

    return NULL;
}

buddy_block_list_t* buddy_split(int start_index, int end_index, int req_size, int curr_order){
    int i,j;
    // to get relative offset as we need to cut it to half while spliting
    int block_offset= end_index-start_index;

    // if the current block is already the smallest block that can be allocated or smaller block is not enough for the request
    if((PAGE_SIZE * (1 << curr_order) / 2) < req_size || curr_order == 0){
        buddy_block_list_t *start = get_block_addr(curr_order,start_index);
        start->order = STATUS_X;
        return start;
    }
    // split the block until the smallest block that can be allocated
    for(i = curr_order - 1; i >= 0; i--){
        buddy_block_list_t *start = get_block_addr(i,start_index);
        buddy_block_list_t *cur = (buddy_block_list_t *)start;

        for(j = start_index; j < start_index + block_offset && cur != 0; j += (1 << i)){
            // mark it as usable
            if(cur->order == STATUS_F){
                cur->order = i;
                if(cur != start){
                    uart_puts("Release redundent block of order:");
                    uart_int(cur->order);
                    uart_puts(" size: ");
                    uart_int(cur->size);
                    uart_puts(" at: ");
                    uart_hex_64((unsigned long long)cur->addr);
                    uart_puts("\n");
                }
            }
            cur = cur->next;
        }
        block_offset /= 2;
        start->order = STATUS_F;
        // if the current block is already the smallest block that can be allocated or smaller block is not enough for the request
        if((PAGE_SIZE * (1 << i) / 2) < req_size || i == 0){
            start->order = STATUS_X;
            return start;
        }
    }
    return NULL;
}

void mark_allocated(buddy_block_list_t* start){
    int i,j;
    int order = log(start->size / PAGE_SIZE, 2);
    int end_index = start->idx + (1 << order);

    for(i = order - 1; i >= 0; i--){
        buddy_block_list_t *cur = get_block_addr(i,start->idx);
        for(j = start->idx; j < end_index && cur != 0; j += (1 << i)){
            if(cur->order == STATUS_F)
                cur->order = STATUS_X;
            cur = cur->next;
        }
    }
}


void buddy_free(void *addr){
    int block_index = (addr-buddy_system->buddy_start_address)/ PAGE_SIZE;
    //don't known order
    buddy_block_list_t *cur_block = get_block_addr(-1,block_index);
    int order = log(cur_block->size / PAGE_SIZE, 2);

    uart_puts("Free :");
    uart_hex_64((unsigned long long)addr);
    uart_puts(" with order:");
    uart_int(log(cur_block->size / PAGE_SIZE, 2));
    uart_puts(" at index:");
    uart_int(block_index);
    uart_send('\n');

    free_child(block_index, order);
    cur_block->order = order;

    //try to merge
    int i,neighbor_same_order_block_index;
    buddy_block_list_t *neighbor_same_order_block;
    for(i = order; i < MAX_ORDER; i++){
        //get previous block whick is always order order i
        neighbor_same_order_block_index = block_index ^ (1 << i);

        cur_block = get_block_addr(i,block_index);
        neighbor_same_order_block = get_block_addr(i,neighbor_same_order_block_index);

        // reaching largest block, no need to merge
        if(i == MAX_ORDER - 1){
            cur_block->order = i;
            break;
        }

        // its neighbor block is also available, combine them into a larger block
        if(neighbor_same_order_block->order >= 0){
            cur_block->order = STATUS_F;
            neighbor_same_order_block->order = STATUS_F;
            uart_puts("Merge two blocks:");
            uart_int(block_index);
            uart_send(' ');
            uart_int(neighbor_same_order_block);
            uart_puts(" at order:");
            uart_int(i);
            uart_send('\n');
        }
        else{
            cur_block->order = i;
            break;
        }
    }
    update_first_avail_list();

}


void free_child(int block_index, int order){
    int i,j;

    for(i = order - 1; i >= 0; i--){
        buddy_block_list_t *cur = get_block_addr(i,block_index);
        for(j = 0; j < (1 << order); j += (1 << i)){
            cur->order = STATUS_F;
            cur = cur->next;
        }
    }
}


void dma_init(void) {
    //small_block_pool = {16, 32, 64, 128, 256, 512, 1024};
    dma->small_block_pools = (small_block_list_t **)simple_malloc(7 * sizeof(small_block_list_t *));  // for sizes 16, 32, 48, etc.
    
    for (int i = 0; i < 10; i++) {
        dma->small_block_pools[i] = NULL;
    }

    buddy_init();
}

void *dma_malloc(int size) {
    if(size>1024){
        return buddy_malloc(size);
    }

    int pool_idx = log(size/16, 2); // Assume sizes are 16, 32, 48, etc.

    if (dma->small_block_pools[pool_idx] != NULL) {
        small_block_list_t *block = dma->small_block_pools[pool_idx];
        dma->small_block_pools[pool_idx] = block->next;
        uart_puts("dynamic malloc a size ");
        uart_int(small_block_pool[pool_idx]);
        uart_puts(" block at: ");
        uart_hex_64(block->addr);
        uart_send('\n');
        return block->addr;
    }


    // Allocate a new page from buddy system
    void *page = buddy_malloc(4096); // Order 0 for a single 4096 page

    for (int i = 0; i < PAGE_SIZE / size; i++) {
        small_block_list_t *block = (small_block_list_t *)simple_malloc(sizeof(small_block_list_t));
        block->addr = page + (i * size);
        block->next = dma->small_block_pools[pool_idx];
        dma->small_block_pools[pool_idx] = block;
        //dma->block_size[addr]=small_block_pool[pool_idx];
    }

    return dma_malloc(size);
}

void dma_free(void *addr, int size) {
    //int size = dma->block_size[addr];
    if (size <= 1024) {
        int pool_idx = log(size/16, 2);
        small_block_list_t *block = (small_block_list_t *)simple_malloc(sizeof(small_block_list_t));
        block->addr = addr;
        block->next = dma->small_block_pools[pool_idx];
        dma->small_block_pools[pool_idx] = block;
    } else {
        buddy_free(addr);
    }
}


void memory_reserve(void* start, void* end){

    if(start < buddy_system->buddy_start_address || end > buddy_system->buddy_end_address){
        uart_puts("Error: The memory is out of range\n");
        return;
    }

    int start_index = (start - buddy_system->buddy_start_address) / PAGE_SIZE;
    int end_index = (end - buddy_system->buddy_start_address) / PAGE_SIZE;
    if((uint64_t)end % PAGE_SIZE != 0)
        end_index++;
    int i;

    if(end_index < start_index){
        uart_puts("Error: The end address is smaller than the start address\n");
        return;
    }

    uart_puts("Reserve memory from:");

    uart_hex_64((unsigned long long)start);
    uart_puts(" to:");
    uart_hex_64((unsigned long long)end);
    uart_send(' ');
    uart_puts("with index:");
    uart_int(start_index);
    uart_send(' ');
    uart_int(end_index);
    uart_send('\n');

    int neighbor_same_order_block_index,cur_index;
    // mark the protect blocks as STATUS_X(allocated)
    for(i = 0; i < MAX_ORDER; i++){
        cur_index = start_index;
        while(cur_index < end_index){ 

            
            buddy_block_list_t *cur = get_block_addr(i,cur_index);
            

            if(cur->order >= 0 || cur->order == STATUS_F || i==0)
                cur->order = STATUS_X;


            neighbor_same_order_block_index = cur_index ^ (1 << i);
            buddy_block_list_t *neighbor_same_order_block = get_block_addr(i,neighbor_same_order_block_index);
            if (neighbor_same_order_block_index < buddy_system->max_index){
                if(neighbor_same_order_block->order == STATUS_F){
                    //uart_int(i);
                    //uart_send(' ');
                    //uart_hex_64(neighbor_same_order_block->addr);
                    //uart_send('\n');
                    neighbor_same_order_block->order = i;    
                }
            }
            
            cur_index++;
        }
    }
    update_first_avail_list();
}


void startup_allocation(void){
    dma_init();
    uart_puts("Availibel pages before startup_allocation\n");
    show_available_page();
    uart_puts("startup_allocation starting\n");
    uart_puts("reserve Spin tables for multicore boot\n");
    memory_reserve((void*)0x0, (void*)0x1000);
    uart_puts("reserve Kernel image in the physical memory\n");
    memory_reserve((void*)&_start, (void*)&__end);
    uart_puts("reserve CPIO archive in the physical memory\n");
    memory_reserve((void*)cpio_addr, (void*)cpio_addr + 0x100000);
    uart_puts("reserve device tree blob in the physical memory\n");
    memory_reserve((void*)dtb_addr, (void*)dtb_addr + 0x100000);
    uart_puts("reserve allocator metadata in the physical memory\n");
    memory_reserve((void*)BUDDY_METADATA_ADDR, (void*)BUDDY_METADATA_ADDR + sizeof(buddy_system_t) + ((1 << MAX_ORDER) - 1) * sizeof(buddy_block_list_t));
    uart_puts("reserve dma metadata in the physical memory\n");
    memory_reserve((void*)&__end, (void*)allocated);
    uart_puts("startup_allocation finished\n");
    uart_puts("Availibel pages after startup_allocation\n");
    show_available_page();
}

void show_available_page(void){
    int avail_count;
    for(int i = 0; i < MAX_ORDER; i++){
        uart_puts("Order ");
        uart_int(i);
        uart_puts(" availible pages number : ");
        avail_count = 0;
        buddy_block_list_t *cur = (buddy_block_list_t *)buddy_system->list_addr[i];
        for(int j = 0; j < (1 << (MAX_ORDER - i - 1)); j++){
            if(cur->order >= 0){
                avail_count++;
            }
            cur = cur->next;
        }
        uart_int(avail_count);
        uart_send('\n');
    }
}

void show_first_available_block_idx(void){
    uart_puts("buddy system first availible block (order 0 -> 10) :\n");
    for(int i = 0; i < MAX_ORDER; i++){
        uart_int(buddy_system->first_avail[i]);
        uart_send(' ');
    }
    uart_send('\n');
}

void memory_reserve_new(void* start, void* end){

    if(start < buddy_system->buddy_start_address || end > buddy_system->buddy_end_address){
        uart_puts("Error: The memory is out of range\n");
        return;
    }

    int start_index = (start - buddy_system->buddy_start_address) / PAGE_SIZE;
    int end_index = (end - buddy_system->buddy_start_address) / PAGE_SIZE;
    int i;

    if(end_index < start_index){
        uart_puts("Error: The end address is smaller than the start address\n");
        return;
    }

    uart_puts("Reserve memory from:");
    uart_hex_64((unsigned long long)start);
    uart_puts(" to:");
    uart_hex_64((unsigned long long)end);
    uart_send(' ');
    uart_puts("with index:");
    uart_int(start_index);
    uart_send(' ');
    uart_int(end_index);
    uart_send('\n');

 
    buddy_block_list_t *cur = get_block_addr(0,0);
    int count=0;
    while(count<buddy_system->max_block_amount){
        if(cur->idx>=start_index && cur->idx<=end_index){
            for(int i=0;i<MAX_ORDER;i++){
                get_block_addr(i,cur->idx)->order=STATUS_X;
            }
        }
        count++;
        cur++;
    }
}
