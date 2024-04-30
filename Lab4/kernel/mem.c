#include "mem.h"

#define HEAP_LIMIT 0x10000000

frame_t* frame_arr;
pool_t*  mem_pool;
index_t* free_frame_list[MAX_ORDER+1];
index_t* free_chunk_list[MAX_CHUNK_OPT];

uint32_t chunk_option[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

static char* heap_head = (char*)&__end;

void* simple_alloc(uint32_t size){

    if ((uint32_t)(heap_head + size) > HEAP_LIMIT){
        print_str("\nNot Enough Memory");
        return (char*)0;
    }

    // print_str("\nAllocating from 0x");
    // print_hex((uint32_t)heap_head);
    // print_str(" to ");
    // print_hex((uint32_t)heap_head+size);

    // print_str("\nLimit Address: 0x");
    // print_hex((uint32_t)HEAP_LIMIT);

    char* alloc_tail = heap_head;
    heap_head += size;

    return alloc_tail;
}

void init_mem(){
    frame_arr = (frame_t*)simple_alloc(MAX_FRAME * sizeof(frame_t));
    mem_pool = (pool_t*)simple_alloc(MAX_FRAME * sizeof(pool_t));

    // init frame_arr
    for (int i = 0; i < MAX_FRAME; i++){
        frame_arr[i].status = FREE;
        
        if (i == 0)
            frame_arr[i].order = MAX_ORDER;
        else
            frame_arr[i].order = -1;

        index_t* id_node = (index_t*)simple_alloc(sizeof(index_t));
        id_node->prev = 0;
        id_node->next = 0;
        id_node->index = i;
        frame_arr[i].id_node = id_node;
    }

    // init mem pool
    for (int i = 0; i < MAX_FRAME; i++){
        
        for (int j = 0; j < MAX_CHUNK_PER_FRAME; j++)
            mem_pool[i].chunk_stat[j] = 0;

        mem_pool[i].chunk_opt = MAX_CHUNK_OPT-1;
        mem_pool[i].free_chunk = 1;
        mem_pool[i].n_chunk = 1;
    }

    // init free frame list
    for (int i = 0; i < MAX_ORDER+1; i++){
        free_frame_list[i] = 0;
    }

    // init mem pool
    for (int i = 0; i < MAX_CHUNK_OPT; i++){
        free_chunk_list[i] = 0;
    }

    free_frame_list[MAX_ORDER] = frame_arr[0].id_node;
}

void insert_frame(uint32_t order, index_t* frame_id_node){

    // async_uart_puts("\n[INSERT FREE FRAME] ID: 0x");
    // async_uart_hex(frame_id_node->index);

    if (free_frame_list[order] == 0){
        free_frame_list[order] = frame_id_node;
        return;
    }

    index_t* cur = free_frame_list[order];

    while (1){
        // async_uart_newline();
        // async_uart_hex(cur->index);

        if (frame_id_node->index > cur->index){
            if (cur->next == 0){
                cur->next = frame_id_node;
                frame_id_node->prev = cur;
                break;
            }else{
                cur = cur->next;
            }
        }else {
            if (cur->prev == 0){
                frame_id_node->next = cur;
                frame_id_node->next->prev = frame_id_node;
                free_frame_list[order] = frame_id_node;
            }else{
                frame_id_node->prev = cur->prev;
                frame_id_node->next = cur;
                frame_id_node->next->prev = frame_id_node;
                frame_id_node->prev->next = frame_id_node;
            }

            // index_t* tmp = free_frame_list[order];
            // while (tmp != 0){
            //     async_uart_newline();
            //     async_uart_hex(tmp->index);
            //     tmp = tmp->next;
            // }
            break;
        }
    }

    // async_uart_newline();
}

index_t* pop_frame(uint32_t order){

    if (free_frame_list[order] == 0){
        async_uart_puts("\n[ERROR] No Free Frame");
        return (index_t*)0;
    }

    index_t* req_id_node = free_frame_list[order];
    free_frame_list[order] = free_frame_list[order]->next;
    free_frame_list[order]->prev = 0;

    // async_uart_puts("\n[POP FREE FRAME] ID: 0x");
    // async_uart_hex(req_id_node->index);

    req_id_node->next = 0;
    req_id_node->prev = 0;

    return req_id_node;
}

void* get_frame(uint32_t size){

    uint32_t target_size = FRAME_SIZE;
    uint32_t target_order = 0;
    
    while (size > target_size){
        target_size = target_size << 1;
        // async_uart_newline();
        // async_uart_hex(target_size);
        target_order++;
        // async_uart_newline();
        // async_uart_hex(target_order);

        if (target_order == MAX_ORDER)
            break;
    }

    // async_uart_puts("\ntarget order: ");
    // async_uart_hex(target_order);

    iterative_split(target_order);

    index_t* frame_id_node = pop_frame(target_order);

    // check_frames();

    if (frame_id_node == 0)
        return (char*)0;
    
    int begin_id = frame_id_node->index;
    int end_id = frame_id_node->index + (1 << target_order);

    // async_uart_puts("\n[ALLOCATE INFO] Allocated Mem: 0x");
    // async_uart_hex(id2Addr(begin_id));
    // async_uart_puts(" - 0x");
    // async_uart_hex(id2Addr(end_id));
    // async_uart_newline();

    for (int i = begin_id; i < end_id; i++){
        frame_arr[i].status = ALLOCATED;
    }

    frame_arr[begin_id].order = target_order;

    void* ptr = (void*)id2Addr(begin_id);

    return ptr;    
}

void* buddy_allocate(uint32_t size){
    uint32_t target_size = FRAME_SIZE;
    uint32_t target_order = 0;
    
    while (size > target_size){
        target_size = target_size << 1;
        // async_uart_newline();
        // async_uart_hex(target_size);
        target_order++;
        // async_uart_newline();
        // async_uart_hex(target_order);

        if (target_order == MAX_ORDER)
            break;
    }

    // async_uart_puts("\ntarget order: ");
    // async_uart_hex(target_order);

    iterative_split(target_order);

    index_t* frame_id_node = pop_frame(target_order);

    // check_frames();

    if (frame_id_node == 0)
        return (char*)0;
    
    int begin_id = frame_id_node->index;
    int end_id = frame_id_node->index + (1 << target_order);

    async_uart_puts("\n[ALLOCATE INFO] Allocated Mem: 0x");
    async_uart_hex(id2Addr(begin_id));
    async_uart_puts(" - 0x");
    async_uart_hex(id2Addr(end_id));
    // async_uart_newline();

    for (int i = begin_id; i < end_id; i++){
        frame_arr[i].status = ALLOCATED;
    }

    frame_arr[begin_id].order = target_order;

    void* ptr = (void*)id2Addr(begin_id);

    return ptr;
        
}

void release_frame(void* ptr){

    uint32_t addr = (uint32_t)ptr;

    uint32_t frame_id = addr2ID(addr);

    if (frame_arr[frame_id].status != ALLOCATED){
        async_uart_puts("\n[FREE ERROR] Address not available: 0x");
        async_uart_hex((uint32_t)ptr);
        // async_uart_newline();
        return;
    }

    uint32_t order = frame_arr[frame_id].order;
    // async_uart_newline();
    // async_uart_hex(order);

    uint32_t nframe = 1 << order;

    for (int i = frame_id; i < frame_id+nframe; i++){
        frame_arr[i].status = FREE;
    }

    index_t* frame_id_node = frame_arr[frame_id].id_node;

    // async_uart_puts("\n[FREE INFO] Free MEM: 0x");
    // async_uart_hex(id2Addr(frame_id));
    // async_uart_puts(" - 0x");
    // async_uart_hex(id2Addr(frame_id+nframe));
    // async_uart_newline();

    insert_frame(order, frame_id_node);

    iterative_merge(order);

    // check_frames();

}

void buddy_free(void* ptr){
    uint32_t addr = (uint32_t)ptr;

    uint32_t frame_id = addr2ID(addr);

    if (frame_arr[frame_id].status != ALLOCATED){
        async_uart_puts("\n[FREE ERROR] Address not available: 0x");
        async_uart_hex((uint32_t)ptr);
        // async_uart_newline();
        return;
    }

    uint32_t order = frame_arr[frame_id].order;
    // async_uart_newline();
    // async_uart_hex(order);

    uint32_t nframe = 1 << order;

    for (int i = frame_id; i < frame_id+nframe; i++){
        frame_arr[i].status = FREE;
    }

    index_t* frame_id_node = frame_arr[frame_id].id_node;

    async_uart_puts("\n[FREE INFO] Free MEM: 0x");
    async_uart_hex(id2Addr(frame_id));
    async_uart_puts(" - 0x");
    async_uart_hex(id2Addr(frame_id+nframe));
    // async_uart_newline();

    insert_frame(order, frame_id_node);

    iterative_merge(order);

    // check_frames();

}

void iterative_split(uint32_t target_order){

    async_uart_newline();

    int free_order = target_order;

    while (free_frame_list[free_order] == 0){
        free_order++;

        if (free_order > MAX_ORDER)
            return;
    }

    while (free_order > target_order){
        index_t* larger_id_node = free_frame_list[free_order];
        uint32_t orig_id = larger_id_node->index;
        uint32_t nframe = (1 << (free_order-1));

        // async_uart_newline();
        // async_uart_hex(free_order);
        // async_uart_newline();
        // async_uart_hex(nframe);

        free_frame_list[free_order] = free_frame_list[free_order]->next;
        
        if (free_frame_list[free_order] != 0){
            free_frame_list[free_order]->prev = 0;
        }

        index_t* smaller_id_node = frame_arr[orig_id+nframe].id_node;
        smaller_id_node->index = orig_id + nframe;

        larger_id_node->next = 0;
        larger_id_node->prev = 0;

        async_uart_puts("\n[ITERATIVE SPLIT] 0x");
        async_uart_hex(id2Addr(larger_id_node->index));
        async_uart_puts("-0x");
        async_uart_hex(id2Addr(smaller_id_node->index));

        async_uart_puts(" & 0x");
        async_uart_hex(id2Addr(smaller_id_node->index));
        async_uart_puts("-0x");
        async_uart_hex(id2Addr(smaller_id_node->index+nframe));

        // async_uart_newline();

        free_order--;

        frame_arr[larger_id_node->index].order = free_order;
        frame_arr[smaller_id_node->index].order = free_order;
        
        insert_frame(free_order, larger_id_node);
        insert_frame(free_order, smaller_id_node);

        // check_frames();
    }

    async_uart_newline();
}

void iterative_merge(uint32_t order){

    uint32_t nframe = (1 << order);

    async_uart_newline();
    // async_uart_hex(req_id)

    while (free_frame_list[order] != 0 && free_frame_list[order]->next != 0) {
        
        index_t* front_id_node = free_frame_list[order];
        index_t* back_id_node = front_id_node->next;

        while (front_id_node != 0 && back_id_node != 0){
            if (front_id_node->index ^ nframe == back_id_node->index){
                
                async_uart_puts("\n[ITERATIVE MERGE] 0x");
                async_uart_hex(id2Addr(front_id_node->index));
                async_uart_puts("-0x");
                async_uart_hex(id2Addr(back_id_node->index));

                async_uart_puts(" & 0x");
                async_uart_hex(id2Addr(back_id_node->index));
                async_uart_puts("-0x");
                async_uart_hex(id2Addr(back_id_node->index+nframe));

                // async_uart_newline();

                if (front_id_node->prev == 0){
                    // async_uart_puts("\nhere");
                    free_frame_list[order] = back_id_node->next;

                    if (free_frame_list[order] != 0)
                        free_frame_list[order]->prev = 0;
                }else {
                    
                    back_id_node->next->prev = front_id_node->prev;
                    front_id_node->prev->next = back_id_node->next;
                }

                frame_arr[front_id_node->index].order = ++order;
                frame_arr[back_id_node->index].order = -1;

                front_id_node->prev = 0;
                front_id_node->next = 0;
                back_id_node->prev = 0;
                back_id_node->next = 0;

                // check_frames();
                insert_frame(order, front_id_node);
                nframe = nframe << 1;

                // async_uart_puts("\n[INFO] Merged Order: ");
                // async_uart_hex(order);
                // async_uart_newline();
                    
                break;
            }else {
                front_id_node = back_id_node;
                back_id_node = back_id_node->next;
            }
        }

        if (order > MAX_ORDER+1)
            break;
    }

    async_uart_newline();

}

void check_frames(){
    async_uart_puts("\n===== FRAME INFO =====");
    for (int i = 0; i <= MAX_ORDER; i++){
        async_uart_newline();
        async_uart_hex(i);
        async_uart_puts(": ");

        index_t* cur = free_frame_list[i];

        while (cur != 0){
            async_uart_hex(cur->index);
            async_uart_puts(" -> ");
            cur = cur->next;
        }

        async_uart_puts("END");
    }

    async_uart_puts("\n========================\n");
}

void insert_chunk(index_t* frame_id_node, uint32_t chunk_opt){

    // async_uart_puts("\n[INSERT FREE CHUNK] 0x");
    // async_uart_hex(id2Addr(frame_id_node->index));
    // async_uart_puts(" with size 0x");
    // async_uart_hex(chunk_option[chunk_opt]);

    while (free_chunk_list[chunk_opt] == 0){
        free_chunk_list[chunk_opt] = frame_id_node;
        return;
    }

    index_t* cur = free_chunk_list[chunk_opt];

    while (1){
        // async_uart_newline();
        // async_uart_hex(cur->index);

        if (frame_id_node->index > cur->index){
            if (cur->next == 0){
                cur->next = frame_id_node;
                frame_id_node->prev = cur;
                break;
            }else{
                cur = cur->next;
            }
        }else {
            if (cur->prev == 0){
                frame_id_node->next = cur;
                frame_id_node->next->prev = frame_id_node;
                free_chunk_list[chunk_opt] = frame_id_node;
            }else{
                frame_id_node->prev = cur->prev;
                frame_id_node->next = cur;
                frame_id_node->next->prev = frame_id_node;
                frame_id_node->prev->next = frame_id_node;
            }

            // index_t* tmp = free_chunk_list[chunk_opt];
            // while (tmp != 0){
            //     async_uart_newline();
            //     async_uart_hex(tmp->index);
            //     tmp = tmp->next;
            // }
            break;
        }
    }

    // async_uart_newline();
    
}

void* pop_chunk(uint32_t chunk_opt){

    if (free_chunk_list[chunk_opt] == 0){
        async_uart_puts("\n[ERROR] No Free Chunk");
        return (void*)0;
    }

    index_t* frame_id_node = free_chunk_list[chunk_opt];
    uint32_t frame_id = frame_id_node->index;

    if (chunk_opt != mem_pool[frame_id].chunk_opt){
        async_uart_puts("\n[ERROR] Wrong chunk option");
        // async_uart_newline();
        // async_uart_hex(chunk_opt);
        // async_uart_newline();
        // async_uart_hex(mem_pool[frame_id].chunk_opt);
        return (void*)0;
    }

    uint32_t chunk_size = chunk_option[chunk_opt];

    uint32_t addr = 0;

    for (int i = 0; i < mem_pool[frame_id].n_chunk; i++){
        if (mem_pool[frame_id].chunk_stat[i] == FREE){
            addr = id2Addr(frame_id) + chunk_size * i;
            mem_pool[frame_id].chunk_stat[i] = ALLOCATED;
            break;
        }
    }

    mem_pool[frame_id].free_chunk--;

    if (mem_pool[frame_id].free_chunk == 0){

        index_t* frame_id_node = free_chunk_list[chunk_opt];
        free_chunk_list[chunk_opt] = frame_id_node->next;

        if (free_chunk_list[chunk_opt] != 0)
            free_chunk_list[chunk_opt]->prev = 0;

        frame_id_node->next = 0;
        frame_id_node->prev = 0;
    }

    // check_chunks();
    // async_uart_puts("\n[ALLOCATE FREE CHUNK] 0x");
    // async_uart_hex(addr);

    return (void*)addr;   
}

void merge_chunk(uint32_t frame_id, uint32_t chunk_opt){
    
    // async_uart_puts("\nhere");
    index_t* cur = free_chunk_list[chunk_opt];
    // check_chunks();

    while (1){
        // async_uart_newline();
        // async_uart_hex(cur->index);

        if (cur->index != frame_id){
            if (cur->next == 0){
                async_uart_puts("\n[ERROR] No chunks to be merged");
                break;
            }else{
                cur = cur->next;
            }
        }else {
            
            index_t* frame_id_node = cur;

            if (cur->prev == 0){
                free_chunk_list[chunk_opt] = cur->next;

                if (free_chunk_list[chunk_opt] != 0)
                    free_chunk_list[chunk_opt]->prev = 0;

            }else{
                // async_uart_puts("\nshould be here");

                cur->prev->next = cur->next;
                
                if (cur->next != 0)
                    cur->next->prev = cur->prev;

            }

            
            frame_id_node->next = 0;
            frame_id_node->prev = 0;

            // async_uart_newline();
            // async_uart_hex(frame_id);

            mem_pool[frame_id].n_chunk = 1;
            async_uart_puts("\r");
            mem_pool[frame_id].chunk_opt = MAX_CHUNK_OPT-1;
            mem_pool[frame_id].free_chunk = 1;
            
            break;
        }
    }

    
}

void* slab_allocate(uint32_t size){
    
    int target_opt = 0;

    while (size > chunk_option[target_opt]){
        target_opt++;

        if (target_opt == MAX_CHUNK_OPT)
            break;
    }

    if (chunk_option[target_opt] == FRAME_SIZE)
        return buddy_allocate(FRAME_SIZE);

    if (free_chunk_list[target_opt] == 0){
        void* frame_ptr = get_frame(FRAME_SIZE);
        
        int chunk_size = chunk_option[target_opt];
        int n_chunk = FRAME_SIZE / chunk_size;

        uint32_t frame_id = addr2ID((uint32_t)frame_ptr);
        // async_uart_newline();
        // async_uart_hex(frame_id);
        mem_pool[frame_id].chunk_opt = target_opt;
        mem_pool[frame_id].n_chunk = n_chunk;
        mem_pool[frame_id].free_chunk = n_chunk;

        for (int i = 0; i < n_chunk; i++)
            mem_pool[frame_id].chunk_stat[i] = FREE;
        
        index_t* frame_id_node = frame_arr[frame_id].id_node;
        insert_chunk(frame_id_node, target_opt);
    }

    // check_chunks();

    void* ptr = pop_chunk(target_opt);

    uint32_t chunk_size = chunk_option[target_opt];

    async_uart_puts("\n[ALLOCATE INFO] Allocated Mem: 0x");
    async_uart_hex((uint32_t)ptr);
    async_uart_puts(" - 0x");
    async_uart_hex((uint32_t)ptr+chunk_size);
    // async_uart_newline();

    return ptr;
}

void slab_free(void* ptr){

    uint32_t addr = (uint32_t)ptr;
    uint32_t frame_id = addr2ID(addr);
    uint32_t chunk_opt = mem_pool[frame_id].chunk_opt;

    uint32_t chunk_id = (addr - id2Addr(frame_id)) / chunk_option[chunk_opt];
    // async_uart_puts("\nFrame ID: 0x");
    // async_uart_hex(frame_id);
    // async_uart_puts("\nChunk ID: 0x");
    // async_uart_hex(chunk_id);
    
    if (mem_pool[frame_id].chunk_stat[chunk_id] == FREE){
        async_uart_puts("\n[FREE ERROR] Address not available: 0x");
        async_uart_hex((uint32_t)ptr);
        async_uart_newline();
        return;
    }

    async_uart_puts("\n[FREE INFO] Free MEM: 0x");
    async_uart_hex(addr);
    async_uart_puts(" - 0x");
    async_uart_hex(addr+chunk_option[chunk_opt]);
    // async_uart_newline();

    mem_pool[frame_id].chunk_stat[chunk_id] = FREE;
    mem_pool[frame_id].free_chunk++;

    // async_uart_puts("\nFree Chunk: 0x");
    // async_uart_hex(mem_pool[frame_id].free_chunk);

    if (mem_pool[frame_id].free_chunk == 1){
        insert_chunk(frame_arr[frame_id].id_node, chunk_opt);
    }else if (mem_pool[frame_id].free_chunk == mem_pool[frame_id].n_chunk){
        merge_chunk(frame_id, chunk_opt);
        void* frame_ptr = (void*)(id2Addr(frame_id));
        release_frame(frame_ptr);
    }
}

void check_chunks(){
    async_uart_puts("\n===== Chunk INFO =====");
    for (int i = 0; i < MAX_CHUNK_OPT; i++){
        async_uart_newline();
        async_uart_hex(i);
        async_uart_puts(": ");

        index_t* cur = free_chunk_list[i];

        while (cur != 0){
            async_uart_hex(cur->index);
            async_uart_puts(" -> ");
            cur = cur->next;
        }

        async_uart_puts("END");
    }

    async_uart_puts("\n========================\n");
}

void* malloc(uint32_t size){
    if (size >= FRAME_SIZE)
        return buddy_allocate(size);
    else return slab_allocate(size);

}

void free(void* ptr){
    uint32_t frame_id = addr2ID((uint32_t)ptr);

    if (mem_pool[frame_id].chunk_opt == MAX_CHUNK_OPT-1)
        buddy_free(ptr);
    else slab_free(ptr);
}

uint32_t addr2ID(uint32_t addr){
    return (addr-MEM_BEGIN) / FRAME_SIZE;
}

uint32_t id2Addr(uint32_t frame_id){
    return MEM_BEGIN + frame_id * FRAME_SIZE;
}