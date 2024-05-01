#include "mem.h"

#define HEAP_LIMIT 0x06000000

frame_t* frame_arr;
pool_t*  mem_pool;
index_t* free_frame_list[MAX_ORDER+1];
index_t* free_chunk_list[MAX_CHUNK_OPT];

uint32_t chunk_option[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

static char* kernel_head = (char*)&__kernel_start;
static char* heap_head = (char*)&__kernel_end;

void* simple_alloc(uint32_t size){

    if ((uint32_t)(heap_head + size) > HEAP_LIMIT){
        print_str("\nNot Enough Memory");
        return (char*)0;
    }

    char* alloc_tail = heap_head;
    heap_head += size;

    return alloc_tail;
}

void init_mem(){
    frame_arr = (frame_t*)simple_alloc(MAX_FRAME * sizeof(frame_t));
    mem_pool = (pool_t*)simple_alloc(MAX_FRAME * sizeof(pool_t));

    // init frame_arr

    async_uart_puts("\nInit Frame Array...");
    for (int i = 0; i < MAX_FRAME; i++){
        frame_arr[i].status = FREE;
        
        if (i == 0)
            frame_arr[i].order = 0;
        else
            frame_arr[i].order = 0xff;

        index_t* id_node = (index_t*)simple_alloc(sizeof(index_t));
        id_node->prev = 0;
        id_node->next = 0;
        id_node->index = i;
        frame_arr[i].id_node = id_node;
    }

    async_uart_puts("\nMem Pool Array...");
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

    async_uart_puts("\nReserving Memory...");

    memory_reserve(0x0, 0x1000);
    memory_reserve((uint32_t)kernel_head, (uint32_t)heap_head);
    memory_reserve(DEVTREE_CPIO_BASE, DEVTREE_CPIO_END);
    memory_reserve(0x30000000, MEM_END);

    async_uart_puts("\nSet up Free Memory...");
    set_free_mem();
    check_frames();
}

void set_free_mem(){

    uint32_t order = 0;
    uint32_t nframe = 1;

    while (1){
        // async_uart_puts("\nOrder: 0x");
        // async_uart_hex(order);
        // async_uart_puts("\nnframe: 0x");
        // async_uart_hex(nframe);
        for (uint32_t frame_id = 0; frame_id < MAX_FRAME; frame_id += 2*nframe){
            uint32_t buddy_id = frame_id ^ nframe;

            // print_str("\n")

            if (frame_arr[frame_id].status == FREE && frame_arr[buddy_id].status == FREE){
                frame_arr[frame_id].order++;
                frame_arr[buddy_id].order = MERGED;

                // async_uart_puts("\n[ITERATIVE MERGE] 0x");
                // async_uart_hex(id2Addr(frame_id));
                // async_uart_puts("-0x");
                // async_uart_hex(id2Addr(buddy_id));

                // async_uart_puts(" & 0x");
                // async_uart_hex(id2Addr(buddy_id));
                // async_uart_puts("-0x");
                // async_uart_hex(id2Addr(buddy_id+nframe));

            }else if (frame_arr[frame_id].status == FREE){
                insert_frame(order, frame_id);
                // check_frames();
            }else if (frame_arr[buddy_id].status == FREE){
                insert_frame(order, buddy_id);
                // check_frames();
            }
            
        }

        nframe = nframe << 1;
        order++;

        if (order > MAX_ORDER)
            break;
    }

}

void insert_frame(uint32_t order, uint32_t frame_id){

    // async_uart_puts("\n[INSERT FREE FRAME] ID: 0x");
    // async_uart_hex(frame_id_node->index);

    index_t* frame_id_node = frame_arr[frame_id].id_node;

    if (free_frame_list[order] == 0){
        free_frame_list[order] = frame_id_node;
    }else{
        frame_id_node->next = free_frame_list[order];
        frame_id_node->next->prev = frame_id_node;
        free_frame_list[order] = frame_id_node;
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

void* buddy_allocate(uint32_t size){

    if (size > MAX_SIZE){
        async_uart_puts("\nSize too big, Not allowed");
        return (void*)0;
    }

    uint32_t target_size = FRAME_SIZE;
    uint32_t target_order = 0;
    
    while (size > target_size){
        target_size = target_size << 1;
        target_order++;

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
    
    int frame_id = frame_id_node->index;
    // async_uart_newline();

    frame_arr[frame_id].status = ALLOCATED;
    frame_arr[frame_id].order = target_order;

    void* mem_ptr = (void*)id2Addr(frame_id);

    return mem_ptr;
        
}

void buddy_free(void* mem_ptr){

    uint32_t frame_id = addr2ID((uint32_t)mem_ptr);

    if (frame_arr[frame_id].status != ALLOCATED){
        async_uart_puts("\n[FREE ERROR] Address not available: 0x");
        async_uart_hex((uint32_t)mem_ptr);
        // async_uart_newline();
        return;
    }

    uint32_t order = frame_arr[frame_id].order;
    uint32_t nframe = 1 << order;
    frame_arr[frame_id].status = FREE;
    // async_uart_newline();

    iterative_merge(order, frame_id);

    // check_frames();

}

void iterative_split(uint32_t target_order){

    // async_uart_newline();

    int free_order = target_order;

    while (free_frame_list[free_order] == 0){
        free_order++;

        if (free_order > MAX_ORDER)
            return;
    }

    while (free_order > target_order){
        index_t* target_id_node = free_frame_list[free_order];

        uint32_t target_id = target_id_node->index;
        uint32_t nframe = (1 << (free_order-1));
        uint32_t buddy_id = target_id + nframe;

        free_frame_list[free_order] = free_frame_list[free_order]->next;
        
        if (free_frame_list[free_order] != 0){
            free_frame_list[free_order]->prev = 0;
        }

        target_id_node->next = 0;
        target_id_node->prev = 0;

        async_uart_puts("\n[ITERATIVE SPLIT] 0x");
        async_uart_hex(id2Addr(target_id));
        async_uart_puts("-0x");
        async_uart_hex(id2Addr(buddy_id));

        async_uart_puts(" & 0x");
        async_uart_hex(id2Addr(buddy_id));
        async_uart_puts("-0x");
        async_uart_hex(id2Addr(buddy_id+nframe));

        // async_uart_newline();

        free_order--;

        frame_arr[target_id].order = free_order;
        frame_arr[buddy_id].order = free_order;
        
        insert_frame(free_order, buddy_id);
        insert_frame(free_order, target_id);

        // check_frames();
    }

    // async_uart_newline();
}

void iterative_merge(uint32_t order, uint32_t frame_id){

    uint32_t target_order = order;
    uint32_t nframe = (1 << target_order);

    uint32_t target_frame_id = frame_id;
    uint32_t target_buddy_id;

    // async_uart_newline();

    while (1){
        uint32_t buddy_id = target_frame_id ^ nframe;

        if (frame_arr[buddy_id].status == ALLOCATED || frame_arr[buddy_id].order != frame_arr[frame_id].order)
            break;

        index_t* buddy_id_node = frame_arr[buddy_id].id_node;
        
        if (buddy_id_node->prev == 0 && buddy_id_node->next == 0){
            free_frame_list[target_order] = 0;
        }else if (buddy_id_node->prev == 0){
            free_frame_list[target_order] = buddy_id_node->next;
            free_frame_list[target_order]->prev = 0;
        }else{
            buddy_id_node->prev->next = buddy_id_node->next;
        }

        buddy_id_node->prev = 0;
        buddy_id_node->next = 0;

        if (target_frame_id > buddy_id){
            target_buddy_id = target_frame_id;
            target_frame_id = buddy_id;
        }else{
            target_buddy_id = buddy_id;
        }

        frame_arr[target_frame_id].order = ++target_order;
        frame_arr[target_buddy_id].order = 0xffffffff;

        async_uart_puts("\n[ITERATIVE MERGE] 0x");
        async_uart_hex(id2Addr(target_frame_id));
        async_uart_puts("-0x");
        async_uart_hex(id2Addr(target_buddy_id));

        async_uart_puts(" & 0x");
        async_uart_hex(id2Addr(target_buddy_id));
        async_uart_puts("-0x");
        async_uart_hex(id2Addr(target_buddy_id+nframe));

        nframe = nframe << 1;

        if (target_order == MAX_ORDER)
            break;
    }

    insert_frame(target_order, target_frame_id);

    // async_uart_newline();

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

void insert_chunk(uint32_t frame_id, uint32_t chunk_opt){

    // async_uart_puts("\n[INSERT FREE CHUNK] 0x");
    // async_uart_hex(id2Addr(frame_id_node->index));
    // async_uart_puts(" with size 0x");
    // async_uart_hex(chunk_option[chunk_opt]);

    index_t* frame_id_node = frame_arr[frame_id].id_node;

    if (free_chunk_list[chunk_opt] == 0){
        free_chunk_list[chunk_opt] = frame_id_node;
    }else{
        frame_id_node->next = free_chunk_list[chunk_opt];
        frame_id_node->next->prev = frame_id_node;
        free_chunk_list[chunk_opt] = frame_id_node;
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

    index_t* frame_id_node = frame_arr[frame_id].id_node;
    
    if (frame_id_node->prev == 0 && frame_id_node->next == 0){
        free_chunk_list[chunk_opt] = 0;
    }else if (frame_id_node->prev == 0){
        free_chunk_list[chunk_opt] = frame_id_node->next;
        free_chunk_list[chunk_opt]->prev = 0;
    }else{
        frame_id_node->prev->next = frame_id_node->next;
    }

    frame_id_node->prev = 0;
    frame_id_node->next = 0;

    // async_uart_puts("\n[MERGE CHUNK] All chunks are free in frame");
    mem_pool[frame_id].n_chunk = 1;
    async_uart_puts("\r");
    mem_pool[frame_id].chunk_opt = MAX_CHUNK_OPT-1;
    mem_pool[frame_id].free_chunk = 1;

    frame_arr[frame_id].status = FREE;
    iterative_merge(0, frame_id);
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
        void* frame_ptr = buddy_allocate(FRAME_SIZE);
        
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
        

        insert_chunk(frame_id, target_opt);
    }

    // check_chunks();

    void* mem_ptr = pop_chunk(target_opt);
    // async_uart_newline();

    return mem_ptr;
}

void slab_free(void* mem_ptr){

    uint32_t addr = (uint32_t)mem_ptr;
    uint32_t frame_id = addr2ID(addr);
    uint32_t chunk_opt = mem_pool[frame_id].chunk_opt;

    uint32_t chunk_id = (addr - id2Addr(frame_id)) / chunk_option[chunk_opt];
    // async_uart_puts("\nFrame ID: 0x");
    // async_uart_hex(frame_id);
    // async_uart_puts("\nChunk ID: 0x");
    // async_uart_hex(chunk_id);
    
    if (mem_pool[frame_id].chunk_stat[chunk_id] == FREE){
        async_uart_puts("\n[FREE ERROR] Address not available: 0x");
        async_uart_hex(addr);
        async_uart_newline();
        return;
    }
    // async_uart_newline();

    mem_pool[frame_id].chunk_stat[chunk_id] = FREE;
    mem_pool[frame_id].free_chunk++;

    // async_uart_puts("\nFree Chunk: 0x");
    // async_uart_hex(mem_pool[frame_id].free_chunk);

    if (mem_pool[frame_id].free_chunk == 1){
        insert_chunk(frame_id, chunk_opt);
    }else if (mem_pool[frame_id].free_chunk == mem_pool[frame_id].n_chunk){
        merge_chunk(frame_id, chunk_opt);
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

    void* mem_ptr;

    if (size >= FRAME_SIZE)
        mem_ptr = buddy_allocate(size);
    else mem_ptr = slab_allocate(size);

    if (mem_ptr == 0)
        return 0;

    uint32_t frame_id = addr2ID((uint32_t)mem_ptr);
    uint32_t mem_size = 0;

    if (frame_arr[frame_id].order == 0 && mem_pool[frame_id].chunk_opt < MAX_CHUNK_OPT-1)
        mem_size = chunk_option[mem_pool[frame_id].chunk_opt];
    else mem_size = (1 << frame_arr[frame_id].order) * FRAME_SIZE;

    async_uart_puts("\n[ALLOCATE INFO] Allocated Mem: 0x");
    async_uart_hex((uint32_t)mem_ptr);
    async_uart_puts(" - 0x");
    async_uart_hex((uint32_t)mem_ptr + mem_size);

    return mem_ptr;
}

void free(void* mem_ptr){

    uint32_t frame_id = addr2ID((uint32_t)mem_ptr);
    uint32_t mem_size = 0;

    if (frame_arr[frame_id].order == 0 && mem_pool[frame_id].chunk_opt < MAX_CHUNK_OPT-1)
        mem_size = chunk_option[mem_pool[frame_id].chunk_opt];
    else mem_size = (1 << frame_arr[frame_id].order) * FRAME_SIZE;    

    if (mem_pool[frame_id].chunk_opt == MAX_CHUNK_OPT-1)
        buddy_free(mem_ptr);
    else slab_free(mem_ptr);

    async_uart_puts("\n[FREE INFO] Free MEM: 0x");
    async_uart_hex((uint32_t)mem_ptr);
    async_uart_puts(" - 0x");
    async_uart_hex((uint32_t)mem_ptr + mem_size);
}

uint32_t addr2ID(uint32_t addr){
    return (addr-MEM_BEGIN) / FRAME_SIZE;
}

uint32_t id2Addr(uint32_t frame_id){
    return MEM_BEGIN + frame_id * FRAME_SIZE;
}

void memory_reserve(uint32_t begin, uint32_t end){
    uint32_t begin_id = addr2ID(begin);
    uint32_t end_id = addr2ID(end + FRAME_SIZE - 1);

    async_uart_puts("\nReserved Mem: 0x");
    async_uart_hex(id2Addr(begin_id));
    async_uart_puts(" - 0x");
    async_uart_hex(id2Addr(end_id));

    for (int i = begin_id; i < end_id; i++){
        frame_arr[i].order = 1;
        frame_arr[i].status = ALLOCATED;
    }
}