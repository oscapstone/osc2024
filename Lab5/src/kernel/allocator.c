#include "kernel/allocator.h"
// get the heap address
char *allocated = (char*)&__end;
int heap_offset = 0;
buddy_system_t *buddy = 0;
void *buddy_mem_start = 0;

int pool_sizes[NUM_POOLS] = {16, 32, 48, 196, 512, 1024};
// lists for each pool
void ***free_lists;
// recorded which pools that page frames belongs to
int *pool_page_addr;
// record the number of free small blocks in each pool
int *free_list_counts;

void mark_allocated(buddy_block_list_t* start);
int get_next_avail(int order);

void* simple_malloc(unsigned int size){
    // 64bits=8bytes
    size += align_offset(size, 8);

    if(heap_offset + size > MAX_HEAP_SIZE)
        return 0;

    // allocate space
    allocated += size;
    // record accumulated allocated space
    heap_offset += size;

    // we need to return the head instead of tail of allocated space
    return (allocated - size);
}
void gdb_mem(){}

void buddy_startup_init(void){
    int i;
    buddy = (buddy_system_t*)BUDDY_METADATA_ADDR;
    buddy->buddy_list = (buddy_block_list_t**)(BUDDY_METADATA_ADDR + sizeof(buddy_system_t));
    // 245760
    unsigned long total_block = (BUDDY_END - BUDDY_START) / PAGE_SIZE;
    //buddy_block_list_t buddy_list[MAX_ORDER];
    // the location where available memory starts(after all metadata)
    //buddy_mem_start = (void*)(BUDDY_START + sizeof(buddy_system_t) + ((1 << MAX_ORDER) - 1) * sizeof(buddy_block_list_t));
    buddy_mem_start = (void*)(BUDDY_START);
    // the head of current size blocks
    buddy_block_list_t *list = (buddy_block_list_t *)(buddy->buddy_list);

    for(i = 0; i < MAX_ORDER; i++){
        int j = 0;
        int alloc_offset = 0;
        int block_amount = (1 << (MAX_ORDER - i - 1));
        
        buddy->first_avail[i] = -1;
        buddy->list_addr[i] = (void*)list;

        buddy_block_list_t *cur = list;
        buddy_block_list_t *last = 0;

        for(j = 0; j < block_amount; j++){
            cur->idx = (j * (1 << i));
            // first, all block except largest one are belonged to the largest continuous memory block
            cur->val = -2;
            if(i == MAX_ORDER - 1){
                cur->val = MAX_ORDER - 1;
            }
            if(cur != buddy->list_addr[0]){
                cur->prev = last;
                cur->prev->next = cur;
            }
            else
                cur->prev = 0;
            cur->next = 0;
            // the address of the memory of corresponding block
            cur->addr = buddy_mem_start + alloc_offset;
            cur->size = PAGE_SIZE * (1 << i);
            // update the offset of buddy system memory
            alloc_offset += PAGE_SIZE * (1 << i);
            
            last = cur;
            // get next block of block list
            cur++;
        }
        // goto next block list
        // here should not be cur += block_amount * sizeof(buddy_block_list_t), as cur is a pointer to buddy_block_list_t(+1 = +sizeof(struct)), this will lead to block_amount*size*size instead of block_amount*size
        list += block_amount;
    }
    // In the begining, only the largest block is available
    buddy->first_avail[MAX_ORDER - 1] = 0;

    int cur_index = 0;
    unsigned long remain_block = total_block;
    // 0x0 - 0x3c000000
    for(i = MAX_ORDER - 1; i >= 0; i--){
        int j;
        buddy_block_list_t *list_cur = (buddy_block_list_t *)((void*)buddy->list_addr[i] + (cur_index / (1 << i) * sizeof(buddy_block_list_t)));

        for(j = cur_index; j < total_block; j+= (1 << i)){
            // if the block is larger than the total block
            if(list_cur->size > remain_block * PAGE_SIZE)
                list_cur->val = -3;
            else{
                remain_block -= (list_cur->size / PAGE_SIZE);
                list_cur->val = i;
                cur_index = list_cur->next->idx;
            }
            
            list_cur = list_cur->next;

            if(remain_block <= 0){
                mark_allocated(list_cur);
                break;
            }
        }

        if(remain_block <= 0)
            break;
    }
    for(i = 0; i < MAX_ORDER; i++)
        buddy->first_avail[i] = get_next_avail(i);
}

void buddy_init(void){
    int i;
    buddy = (buddy_system_t*)BUDDY_METADATA_ADDR;
    buddy->buddy_list = (buddy_block_list_t**)(BUDDY_METADATA_ADDR + sizeof(buddy_system_t));

    //buddy_block_list_t buddy_list[MAX_ORDER];
    // the location where available memory starts(after all metadata)
    //buddy_mem_start = (void*)(BUDDY_START + sizeof(buddy_system_t) + ((1 << MAX_ORDER) - 1) * sizeof(buddy_block_list_t));
    buddy_mem_start = (void*)(BUDDY_START);
    // the head of current size blocks
    buddy_block_list_t *list = (buddy_block_list_t *)(buddy->buddy_list);

    for(i = 0; i < MAX_ORDER; i++){
        int j = 0;
        int alloc_offset = 0;
        int block_amount = (1 << (MAX_ORDER - i - 1));
        
        buddy->first_avail[i] = -1;
        buddy->list_addr[i] = (void*)list;

        buddy_block_list_t *cur = list;
        buddy_block_list_t *last = 0;

        for(j = 0; j < block_amount; j++){
            cur->idx = (j * (1 << i));
            // first, all block except largest one are belonged to the largest continuous memory block
            cur->val = -2;
            if(i == MAX_ORDER - 1){
                cur->val = MAX_ORDER - 1;
            }
            if(cur != buddy->list_addr[0]){
                cur->prev = last;
                cur->prev->next = cur;
            }
            else
                cur->prev = 0;
            cur->next = 0;
            // the address of the memory of corresponding block
            cur->addr = buddy_mem_start + alloc_offset;
            cur->size = PAGE_SIZE * (1 << i);
            // update the offset of buddy system memory
            alloc_offset += PAGE_SIZE * (1 << i);
            
            last = cur;
            // get next block of block list
            cur++;
        }
        gdb_mem();
        // goto next block list
        // here should not be cur += block_amount * sizeof(buddy_block_list_t), as cur is a pointer to buddy_block_list_t(+1 = +sizeof(struct)), this will lead to block_amount*size*size instead of block_amount*size
        list += block_amount;
    }
    // In the begining, only the largest block is available
    buddy->first_avail[MAX_ORDER - 1] = 0;
}

void show_mem_stat(void){
    int i;
    int avail_pages = 0;
    for(i = 0; i < MAX_ORDER; i++){
        uart_puts("Order:");
        uart_itoa(i);
        uart_puts(" First Available index:");
        uart_itoa(buddy->first_avail[i]);
        uart_puts(" Available Blocks:");
        int avails = 0;
        buddy_block_list_t *cur = (buddy_block_list_t *)buddy->list_addr[i];
        int j;
        int block_amount = (1 << (MAX_ORDER - i - 1));
        for(j = 0; j < block_amount; j++){
            if(cur->val >= 0){
                avails++;
                uart_itoa(j * (1 << i));
                uart_putc(' ');
                avail_pages += (1 << i);
            }
            cur = cur->next;
        }
        uart_itoa(avails);
        uart_putc('\n');
    }
    //uart_putc('\n');
    uart_puts("Total available Pages:");
    uart_itoa(avail_pages);
    uart_putc('\n');
}
void show_buddy_system_stat(void){
    int i;
    for(i = 0; i < MAX_ORDER; i++){
        int j = 0;
        int block_amount = (1 << (MAX_ORDER - i - 1));
        buddy_block_list_t *list_cur = buddy->list_addr[i];
        for(j = 0; j < block_amount; j++){

            uart_itoa(list_cur->val);
            uart_putc(' ');
            list_cur = list_cur->next;
        }
        uart_putc('\n');
    }
}

// get buddy of one level lower order
int find_buddy(int index, int order){
    return index ^ (1 << order);
}
// get the index of the next available block
int get_next_avail(int order){
    int i;
    buddy_block_list_t *cur = (buddy_block_list_t *)buddy->list_addr[order];
    for(i = 0; i < (1 << (MAX_ORDER - 1)); i += (1 << order)){
        if(cur->val >= 0)
            return i;
        cur = cur->next;
    }
    return -1;
}
// get block index by using address
int get_index(void *addr){
    return (addr - buddy_mem_start) / PAGE_SIZE;
}
// get block metadata address by using index
buddy_block_list_t* get_block(int index){
    buddy_block_list_t *cur = (buddy_block_list_t *)((void*)buddy->list_addr[0] + index * sizeof(buddy_block_list_t));
    buddy_block_list_t *last = cur;
    for(int i = 0; i < MAX_ORDER; i++){
        cur = (buddy_block_list_t *)((void*)buddy->list_addr[i] + (index / (1 << i)) * sizeof(buddy_block_list_t));
        // this block index is not legal in this level, indicate that it must belonged to lower levels
        if(index % (1 << i) != 0)
            return last;
        // if larger block is not -1, meaning that we reach the target block as larger block will also make smaller ones become -1
        if(cur->val != -1)
            return last;
        last = cur;
    }

    return cur;
}

buddy_block_list_t* buddy_split(int start_index, int end_index, int req_size, int order){
    int i;
    // to get relative offset as we need to cut it to half while spliting
    end_index -= start_index;
    // if the current block is already the smallest block that can be allocated or smaller block is not enough for the request
    if((PAGE_SIZE * (1 << order) / 2) < req_size || order == 0){
        //buddy->first_avail[order] = get_next_avail(order);
        buddy_block_list_t *start = (buddy_block_list_t *)((void*)buddy->list_addr[order] + (start_index / (1 << order)) * sizeof(buddy_block_list_t));
        // mark it as allocated
        start->val = -1;
        return start;
    }
    // split the block until the smallest block that can be allocated
    for(i = order - 1; i >= 0; i--){
        int j = start_index;
        buddy_block_list_t *start = (buddy_block_list_t *)((void*)buddy->list_addr[i] + (start_index / (1 << i)) * sizeof(buddy_block_list_t));
        buddy_block_list_t *cur = (buddy_block_list_t *)start;

        for(; j < start_index + end_index && cur != 0; j += (1 << i)){
            // mark it as usable
            if(cur->val == -2){
                cur->val = i;
                if(cur != start){
                    uart_puts("Release redundent block:");
                    uart_itoa(cur->size);
                    uart_puts(" at: ");
                    uart_b2x_64((unsigned long long)cur->addr);
                    uart_putc('\n');
                }
                //buddy->first_avail[i] = get_next_avail(i);
            }
            //uart_b2x_64((unsigned long long)cur->val);
            //uart_putc(' ');
            cur = cur->next;
        }
        //uart_putc('\n');
        end_index /= 2;

        start->val = -3;
        // if the current block is already the smallest block that can be allocated or smaller block is not enough for the request
        if((PAGE_SIZE * (1 << i) / 2) < req_size || i == 0){
            start->val = -1;
            //buddy->first_avail[i] = get_next_avail(i);
            return start;
        }
        
        // mark current block as allocated(as its lower level block will be allocated in later iterations)
        //start->val = -1;
    }
    /*if(order == 0)
        return (buddy_block_list_t *)buddy->list_addr[0] + start_index * sizeof(buddy_block_list_t);*/

    return 0;
}
// mark the lower blocks within same range as allocated
void mark_allocated(buddy_block_list_t* start){
    int i;
    int order = simple_log(start->size / PAGE_SIZE, 2);
    int end_index = start->idx + (1 << order);
    for(i = order - 1; i >= 0; i--){
        int j = start->idx;
        buddy_block_list_t *cur = (buddy_block_list_t *)((void*)buddy->list_addr[i] + (start->idx / (1 << i)) * sizeof(buddy_block_list_t));
        for(; j < end_index && cur != 0; j += (1 << i)){
            if(cur->val == -2)
                cur->val = -1;
            cur = cur->next;
        }
    }
}

void free_child(int block_index, int order){
    int i;

    for(i = order - 1; i >= 0; i--){
        int j;
        buddy_block_list_t *cur = (buddy_block_list_t *)((void*)buddy->list_addr[i] + (block_index / (1 << i)) * sizeof(buddy_block_list_t));
        for(j = 0; j < (1 << order); j += (1 << i)){
            cur->val = -2;
            cur = cur->next;
        }
    }
}

// always allocate first block of smallest fitted size
void* buddy_malloc(unsigned int size){
    int i;

    if(buddy == 0)
        buddy_init();

    if(size > PAGE_SIZE * (1 << (MAX_ORDER - 1))){
        uart_puts("Requested size is too large\n");
        return 0;
    }


    for(i = 0; i < MAX_ORDER; i++){
        if(PAGE_SIZE * (1 << i) >= size){
            if(buddy->first_avail[i] >= 0){
                buddy_block_list_t *cur = (buddy_block_list_t *)((void*)buddy->list_addr[i] + (buddy->first_avail[i] / (1 << i)) * sizeof(buddy_block_list_t));
                //buddy_block_list_t *cur = (buddy_block_list_t *)buddy->first_avail[i];
                // meaning that the mechanism went wrong, this condition should not be met
                if(cur->val < 0){
                    uart_puts("Error: The first available block is already allocated\n");
                    return 0;
                }
                // first assuming that there's smaller block in lower layer
                cur->val = -3;

                int avail_index = buddy->first_avail[i];

                //split the block as small as possible
                buddy_block_list_t *buddy_allocated = buddy_split(avail_index, avail_index + (1 << i), size, i);
                mark_allocated(buddy_allocated);

                //show_buddy_system_stat();
                int k;
                // update the first available block
                for(k = 0; k < MAX_ORDER; k++){
                    buddy->first_avail[k] = get_next_avail(k);
                    uart_itoa(buddy->first_avail[k]);
                    uart_putc(' ');
                }
                uart_putc('\n');

                uart_puts("Allocated block size:");
                uart_itoa(buddy_allocated->size);
                uart_puts(" at: ");
                uart_b2x_64((unsigned long long)buddy_allocated->addr);
                uart_putc('\n');
                return (void*)(buddy_allocated->addr);
            }
            else{
                continue;
            }
        }
    }

    return 0;
}

void buddy_free(void *addr){
    int block_index = get_index(addr);
    buddy_block_list_t *cur_block = get_block(block_index);
    int order = simple_log(cur_block->size / PAGE_SIZE, 2);
    int buddy_index = find_buddy(block_index, order);
    buddy_block_list_t *buddy_block = (buddy_block_list_t *)((void*)buddy->list_addr[order] + (buddy_index / (1 << order)) * sizeof(buddy_block_list_t));
    /*uart_itoa(cur_block->size);
    uart_putc(' ');
    uart_itoa(block_index);
    uart_putc(' ');
    uart_b2x_64((unsigned long long)cur_block->addr);
    uart_putc(' ');
    uart_itoa(order);
    uart_putc(' ');
    uart_itoa(buddy_index);
    uart_putc('\n');*/
    uart_puts("Free :");
    uart_b2x_64((unsigned long long)addr);
    uart_puts(" with size:");
    uart_itoa(cur_block->size);
    uart_puts(" at index:");
    uart_itoa(block_index);
    uart_putc('\n');

    free_child(block_index, order);
    // First assuming no merge is needed
    cur_block->val = order;
    int i;
    for(i = order; i < MAX_ORDER; i++){
        // To prevet the case that block index is not a block in larger block level
        //block_index = find_min(block_index, buddy_index);
        buddy_index = find_buddy(block_index, i);
        cur_block = (buddy_block_list_t *)((void*)buddy->list_addr[i] + (block_index / (1 << i)) * sizeof(buddy_block_list_t));
        buddy_block = (buddy_block_list_t *)((void*)buddy->list_addr[i] + (buddy_index / (1 << i)) * sizeof(buddy_block_list_t));

        // reaching largest block, no need to merge
        if(i == MAX_ORDER - 1){
            cur_block->val = i;;
            break;
        }

        // its buddy is also available, combine them into a larger block
        if(buddy_block->val >= 0){
            cur_block->val = -2;
            buddy_block->val = -2;
            uart_puts("Merge two blocks:");
            uart_itoa(block_index);
            uart_putc(' ');
            uart_itoa(buddy_index);
            uart_puts(" at order:");
            uart_itoa(i);
            uart_putc('\n');
        }
        else{
            cur_block->val = i;
            break;
        }
    }

    // update the first available block
    for(i = 0; i < MAX_ORDER; i++){
        buddy->first_avail[i] = get_next_avail(i);
        uart_itoa(buddy->first_avail[i]);
        uart_putc(' ');
    }
    uart_putc('\n');

    //show_buddy_system_stat();
}

void pool_init() {
    int i;
    free_lists = simple_malloc(sizeof(void *) * NUM_POOLS * MAX_CHUNKS_PER_POOL);
    for(i = 0; i < NUM_POOLS; i++){
        free_lists[i] = (void**)simple_malloc(sizeof(void *) * MAX_CHUNKS_PER_POOL);
    }
    gdb_mem();
    pool_page_addr = simple_malloc(sizeof(int) * (1 << (MAX_ORDER - 1)));
    free_list_counts = simple_malloc(sizeof(int) * NUM_POOLS);

    if(free_lists == 0 || pool_page_addr == 0 || free_list_counts == 0){
        uart_puts("Error:Simple memory allocation failed\n");
        return;
    }
    
    for(i = 0; i < NUM_POOLS; i++){
        free_list_counts[i] = 0;
    }
    for(i = 0; i < (1 << (MAX_ORDER - 1)); i++){
        pool_page_addr[i] = -1;
    }
}

void *pool_alloc(unsigned int size) {
    int pool_idx;
    for(pool_idx = 0; pool_idx < NUM_POOLS; pool_idx++){
        if (pool_sizes[pool_idx] >= size)
            break;
    }

    // Exceed pool size, give it a whole page frame
    if(pool_idx == NUM_POOLS)
        return buddy_malloc(size);

    if(free_list_counts[pool_idx] <= 0) {
        // Allocate a new page frame from the buddy system
        void *page = buddy_malloc(4096); //4K page(min. in buddy system)
        if(page == 0){
            uart_puts("No available memory\n");
            return 0;
        }

        int num_chunks = PAGE_SIZE / pool_sizes[pool_idx];
        void *chunks = page;

        pool_page_addr[get_index(page)] = pool_idx;
        // Initialize the free list for the new page
        for (int i = 0; i < num_chunks; i++) {
            // i * pool_sizes[pool_idx] * char(1byte) to get the address of the chunk
            free_lists[pool_idx][free_list_counts[pool_idx]++] = &((char *)chunks)[i * pool_sizes[pool_idx]];
            //uart_b2x_64((unsigned long long)free_lists[pool_idx][free_list_counts[pool_idx] - 1]);
            //uart_putc(' ');
        }
        uart_putc('\n');

        uart_puts("Allocated memory pool size: ");
        uart_itoa(pool_sizes[pool_idx]);
        uart_puts(" at page frame index: ");
        uart_itoa(get_index(page));
        uart_putc('\n');
    }

    int cur_idx = free_list_counts[pool_idx] - 1;
    // Allocate a chunk from the free list
    void *chunk = free_lists[pool_idx][--free_list_counts[pool_idx]];

    uart_puts("Allocated memory pool size: ");
    uart_itoa(pool_sizes[pool_idx]);
    uart_puts(" at address: ");
    uart_b2x_64((unsigned long long)chunk);
    uart_puts(" ,with index: ");
    uart_itoa(cur_idx);
    uart_putc('\n');

    return chunk;
}

void pool_free(void *ptr){
    // Determine the pool based on the address
    int pool_idx = -1;
    int offset = (char *)ptr - (char *)BUDDY_START;
    if(offset < PAGE_SIZE * (1 << (MAX_ORDER - 1))){
        // get the index of the block, then get the pool index
        pool_idx = pool_page_addr[offset / PAGE_SIZE];
    }

    if(pool_idx == -1){
        buddy_free(ptr);
        return; // Invalid pointer or it's a whole page
    }

    uart_puts("Freed memory pool with index: ");
    uart_itoa(free_list_counts[pool_idx]);
    uart_putc('\n');

    free_lists[pool_idx][free_list_counts[pool_idx]++] = ptr;
}

void memory_reserve(void* start,void* end){
    if(start < (void*)BUDDY_START || end > (void*)BUDDY_END){
        uart_puts("Error: The memory is out of range\n");
        return;
    }

    int start_index = get_index(start);
    int end_index = get_index(end);
    if((my_uint64_t)end % PAGE_SIZE != 0)
        end_index++;
    int i;

    if(end_index < start_index){
        uart_puts("Error: The end address is smaller than the start address\n");
        return;
    }

    uart_puts("Reserve memory from:");
    uart_b2x_64((unsigned long long)start);
    uart_puts(" to:");
    uart_b2x_64((unsigned long long)end);
    uart_putc(' ');
    uart_puts("with index:");
    uart_itoa(start_index);
    uart_putc(' ');
    uart_itoa(end_index);
    uart_putc('\n');

    // mark lowest level(0) blocks as allocated(-1), higher level blocks as dividing into smaller blocks(-3) 
    for(i = 0; i < MAX_ORDER; i++){
        int j = start_index;
        do{
            buddy_block_list_t *cur = (buddy_block_list_t *)((void*)buddy->list_addr[i] + (j / (1 << i)) * sizeof(buddy_block_list_t));
            buddy_block_list_t *buddy_block = (buddy_block_list_t *)((void*)buddy->list_addr[i] + (find_buddy(j, i) / (1 << i)) * sizeof(buddy_block_list_t));
            if(cur->val == -1){
                uart_puts("Warning: The block is already allocated(reserved) in:");
                uart_b2x_64((unsigned long long)cur->addr);
                uart_putc('\n');
                //uart_getc();
                //return;
            }

            if(cur->val >= 0 || cur->val == -2)
                cur->val = -3;
            // if buddy is not allocated, mark it as usable as higher level will be marked as -3
            if(buddy_block->val == -2)
                buddy_block->val = i;    
            if(i == 0)
                cur->val = -1;
            j++;
        }while(j < end_index);
    }
    // update the first available block
    for(i = 0; i < MAX_ORDER; i++){
        buddy->first_avail[i] = get_next_avail(i);
        //uart_itoa(buddy->first_avail[i]);
        //uart_putc(' ');
    }
}

void startup_init(void){
    uart_puts("Buddy System init starting\n");
    //buddy_init();
    buddy_startup_init();
    uart_puts("Pool init starting\n");
    pool_init();
    uart_puts("Startup init starting\n");
    //show_mem_stat();
    //memory_reserve((void*)0x10000000, (void*)buddy->list_addr[MAX_ORDER - 1] + sizeof(buddy_block_list_t));
    //show_mem_stat();
    // reserve Spin tables for multicore boot
    memory_reserve((void*)0x0, (void*)0x1000);
    //show_mem_stat();
    memory_reserve((void*)0x1000, (void*)0x80000);
    //show_mem_stat();
    // reserve Kernel image in the physical memory
    memory_reserve((void*)&_start, (void*)&__end);
    //show_mem_stat();
    // reserve the CPIO archive in the physical memory
    memory_reserve((void*)cpio_addr, (void*)cpio_end);
    //show_mem_stat();
    // reserve the device tree blob in the physical memory
    memory_reserve((void*)_dtb_addr, (void*)_dtb_addr + 0x30000);
    //show_mem_stat();
    // reserve allocator metadata in the physical memory
    memory_reserve((void*)BUDDY_METADATA_ADDR, (void*)BUDDY_METADATA_ADDR + sizeof(buddy_system_t) + ((1 << MAX_ORDER) - 1) * sizeof(buddy_block_list_t));
    //show_mem_stat();
    // reserve the pool metadata in the physical memory
    memory_reserve((void*)&__end, (void*)allocated + 0x100000);
    //show_mem_stat();
}