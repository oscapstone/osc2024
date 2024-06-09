#include "alloc.h"
#include "mem.h"
#include "io.h"
#include "lib.h"

#define UNUSED(x) (void)(x)

static char* HEAP_HEADER;
static char* HEAP_REAR;

void  mem_init()
{
    HEAP_HEADER = (char*)&_heap_start;
    HEAP_REAR = HEAP_HEADER;
}

void* simple_malloc(uint32_t size)
{
    if((void*)HEAP_REAR + size > (void*)HEAP_END){

        return (char*)0;
    }
    char* ret = HEAP_REAR;
    HEAP_REAR += size;
    return ret;
}

// ==========================================

static uint64_t pow_2(uint32_t n);

extern uint64_t  _kernel_end;
static uint64_t* _kernel_end_ptr = &_kernel_end;
static uint64_t* _heap_start_ptr = &_heap_start;

#ifndef QEMU
extern uint64_t CPIO_START_ADDR_FROM_DT;
extern uint64_t CPIO_END_ADDR_FROM_DT;
#endif

struct frame_t{
    struct frame_t* next;
    struct frame_t* prev; // [TODO] maintain prev pointer
    uint32_t index; 
    uint8_t level;
    uint8_t status;
};

struct flist_t{
    struct frame_t* head;
};

struct buddy_system_list_t{
    void* base_addr;
    uint32_t index;
    uint8_t max_level;
};

struct memory_pool_t{
    uint8_t frame_used;
    uint8_t chunk_used;
    uint32_t size;
    uint32_t chunk_per_frame;
    uint32_t chunk_offset;  // use to get the offset of chunk in a frame
    struct chunk_t* free_chunk;
    void *frame_base_addrs[MEMORY_POOL_DEPTH];
};

struct chunk_t{
    struct chunk_t* next;
    struct chunk_t* prev;
};

static struct frame_t frame_arr[FRAME_NUM];
static struct flist_t flist_arr[MAX_LEVEL + 1];
static struct memory_pool_t memory_pools[MEMORY_POOL_SIZE];
static struct buddy_system_list_t buddy_system_list[MAX_BUDDY_SYSTEM_LIST];

#define FREE                 0
#define ALLOCATED            1
#define LARGE_CONTIGUOUS     2
#define RESERVED             3


static void add_to_flist(struct flist_t* flist, struct frame_t* frame)
{
    struct frame_t *curr = flist->head;
    while(curr)
    {
        if(curr->next)curr = curr->next;
        else break;
    }
    if(curr) curr->next = frame;
    else 
    {
        // printf("\r\n[SYSTEM INFO] Add Frame: "); printf_int(frame->index); printf(" to Level: "); printf_int(frame->level);
        flist->head = frame;
        // printf("\r\n[SYSTEM INFO] Frame status: "); printf_int(frame->status);
    }
#ifdef DEBUG
    printf("\r\n[DEBUG] Leave add_to_flist");
#endif
}

static void remove_from_flist(struct flist_t* flist, struct frame_t* frame)
{
    struct frame_t *curr = flist->head;
    struct frame_t *prev = NULL;
    int next_index = frame_arr[frame->index + pow_2(frame->level) - 1].next == NULL \
                        ? -1 : frame_arr[frame->index + pow_2(frame->level) - 1].next->index;
    frame_arr[frame->index + pow_2(frame->level) - 1].next = NULL;
    while(curr)
    {
        if(curr == frame)
        {
            if(prev) prev->next = next_index == -1 ? NULL : &frame_arr[next_index];
            else flist->head = next_index == -1 ? NULL : &frame_arr[next_index];
#ifdef DEBUG
            printf("\r\n[DEBUG] Remove Frame: "); printf_int(frame->index); printf(" from flist Level: "); printf_int(frame->level);
#endif
            // curr->next = NULL;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
#ifdef DEBUG
    printf("\r\n[DEBUG ERROR] Frame Not Found in flist!");
#endif
    return;
}

static uint64_t pow_2(uint32_t n)
{
    return 1 << n;
}

void memory_reserve(uint64_t start, uint64_t end, char* msg)
{
#ifdef DEBUG
    printf("\r\n[SYSTEM INFO] Reserve Memory from: "); printf_hex(start); printf(" to: "); printf_hex(end);
#endif
    uint32_t start_index = (start - 0) >> 12;
    uint32_t end_index = (end - 0) >> 12;

#ifdef DEBUG
    printf("\r\n[SYSTEM INFO] Reserve Memory from Frame: "); printf_int(start_index); printf(" to Frame: "); printf_int(end_index);
    printf(" for "); printf(msg);
#endif
    if(start_index == end_index) 
        frame_arr[start_index].status = RESERVED;
    else
        for(int i=start_index; i<end_index; i++) { frame_arr[i].status = RESERVED; }
}

static uint8_t get_lower_bound_level(uint32_t cnt)
{
    uint8_t level = 0;
    while(pow_2(level) < cnt) level++;
    return level-1;
}

static uint32_t get_bid(uint64_t addr)
{
    for(int i=0; i<MAX_BUDDY_SYSTEM_LIST-1; i++)
    {
        if(buddy_system_list[i+1].base_addr == NULL && addr >= (uint64_t)buddy_system_list[i].base_addr)
        {
            return i;
        }
        else{
            if( addr >= (uint64_t)buddy_system_list[i].base_addr 
                && addr < (uint64_t)buddy_system_list[i+1].base_addr )
            {
                return i;
            }
        }
    }
    return -1;
}

void print_bslist(int argc, char* argv[])
{
    printf("\r\n========== buddy system list ==========");
    for(int i=0; i<MAX_BUDDY_SYSTEM_LIST; i++)
    {
        if(buddy_system_list[i].base_addr == NULL) break;
        printf("\r\n[SYSTEM INFO] Start Frame: "); printf_int(buddy_system_list[i].index);
        printf(", Base Address: "); printf_hex((uint64_t)buddy_system_list[i].base_addr);
        printf(", Max Level: "); printf_int(buddy_system_list[i].max_level);
    }
}

void frame_init_with_reserve()
{
    for(uint32_t i=0; i<FRAME_NUM; i++) { frame_arr[i].status = LARGE_CONTIGUOUS; frame_arr[i].level = 0;}
    memory_reserve(0x00000000, 0x00001000, "Spin Table"); // reserve for spin table
    memory_reserve(0x00080000, (uint64_t)(void*)_kernel_end_ptr, "Kernel Image"); // kernel Image
    memory_reserve((uint64_t)(void*)_heap_start_ptr, HEAP_END, "Simple Malloc"); // reserve for simple_malloc
#ifndef QEMU
    memory_reserve(CPIO_START_ADDR_FROM_DT, CPIO_END_ADDR_FROM_DT, "Initramfs"); // reserve for initramfs
#else
    memory_reserve(0x20000000, 0x20000400, "Test Initramfs");
#endif
    uint8_t bid = 0;
    for(int i=0; i<MAX_BUDDY_SYSTEM_LIST; i++) { buddy_system_list[i].base_addr = NULL; buddy_system_list[i].max_level = 0; }
    for(uint8_t i=0; i<=MAX_LEVEL; i++) { flist_arr[i].head = NULL; }

    int is_free_start = 1;
    int free_cnt = 0;
    struct frame_t* free_start = NULL;
#ifdef DEBUG
    printf("\r\n[DEBUG] Frame NUM: "); printf_int(FRAME_NUM);
    printf("\r\n[DEBUG] free_start: "); printf_hex((uint64_t)free_start);
#endif
    for(uint32_t i=0; i<FRAME_NUM; i++)
    {
        frame_arr[i].index = i;
        
        if(frame_arr[i].status != RESERVED)
        {
            if(i<FRAME_NUM-1 && (frame_arr[i].status == frame_arr[i+1].status)){
                frame_arr[i].next = &frame_arr[i+1];
            }
            else{
#ifdef DEBUG
                printf("\r\n[DEBUG] Frame: "); printf_int(i); printf(", next is NULL");
#endif
                frame_arr[i].next = NULL;
            }
        }

        if(frame_arr[i].status == RESERVED){
            if(is_free_start==0 && free_start != NULL){
                free_start->level = get_lower_bound_level(free_cnt);
#ifdef DEBUG
                printf("\r\n[DEBUG] Free Start Frame: "); printf_int(free_start->index);
                printf(", Level: "); printf_int(free_start->level);
#endif
                buddy_system_list[bid].index = free_start->index;
                buddy_system_list[bid].base_addr = (void*)(uint64_t)(MALLOC_START_ADDR + free_start->index * FRAME_SIZE);
                buddy_system_list[bid++].max_level = free_start->level;
                frame_arr[free_start->index + pow_2(free_start->level) - 1].next = NULL;
                add_to_flist(&flist_arr[free_start->level], free_start);
                is_free_start = 1;
                free_cnt = 0;
                free_start = NULL;
            }
        }
        else{
            if(is_free_start){
                buddy_system_list[bid].base_addr = &frame_arr[i];
                frame_arr[i].status = FREE;
                free_start = &frame_arr[i];
                is_free_start = 0;
                free_cnt = 1;
            }
            else{
                frame_arr[i].status = LARGE_CONTIGUOUS;
                frame_arr[i].level = 0;
                free_cnt++;
            }
        }
        if(i== FRAME_NUM-1 && is_free_start==0 && free_start != NULL){
            free_start->level = get_lower_bound_level(free_cnt);
#ifdef DEBUG
            printf("\r\n[DEBUG] Free Start Frame: "); printf_int(free_start->index);
            printf(", Level: "); printf_int(free_start->level); printf(" "); printf_int(free_cnt);
#endif
            buddy_system_list[bid].index = free_start->index;
            buddy_system_list[bid].base_addr = (void*)(uint64_t)(MALLOC_START_ADDR + free_start->index * FRAME_SIZE);
            buddy_system_list[bid++].max_level = free_start->level;
            frame_arr[free_start->index + pow_2(free_start->level) - 1].next = NULL;
            add_to_flist(&flist_arr[free_start->level], free_start);
            is_free_start = 1;
            free_cnt = 0;
            free_start = NULL;
        }
    }

#ifdef DEBUG
    int tag = 0;
    printf("\r\n[SYSTEM INFO] Buddy System List (Start Frame, base_addr, max_level): \n");
#endif
    for(int i=0; i<bid; i++)
    {
#ifdef DEBUG
        if(tag)printf(" -> ");
#endif
        if(buddy_system_list[i].base_addr == NULL) break;
#ifdef DEBUG
        printf("( "); printf_int(buddy_system_list[i].index); printf(", "); 
        printf_hex((uint64_t)buddy_system_list[i].base_addr); printf(", "); 
        printf_int(buddy_system_list[i].max_level); printf(" )");
        tag = 1;
#endif
    }
}

void print_flist(int argc, char* argv[])
{
    printf("\r\n========== free list ==========");
    for(int i=0; i<=MAX_LEVEL; i++)
    {
#ifdef DEBUG
        printf("\r\n[DEBUG] Level: ");printf_int(i);
#endif
        struct frame_t* frame = flist_arr[i].head;
        int sign = 0;
        while(frame != NULL)
        {
            if(frame->status == FREE)
            {
                if(sign == 0) { printf("\r\n[SYSTEM INFO] Level: "); printf_int(i); printf(", Frame: "); }
                if(sign) printf("-> ");
                printf_int(frame->index);
                sign = 1;
                // frame = frame->next;
                frame = frame_arr[frame->index + pow_2(frame->level) - 1].next;
#ifdef DEBUG
                if(frame != NULL) {printf(" (expected next: "); printf_int(frame->index); printf(")");}
#endif
            }
        }
#ifdef DEBUG
        printf("\r\n[DEBUG] Leave Level: ");printf_int(i);
#endif
    }
#ifdef DEBUG
    printf("\r\n[DEBUG] Leave print_flist");
#endif
}

void print_allocated(int argc, char* argv[])
{
    printf("\r\n=========== allocated ===========");
    for(int i=0; i<FRAME_NUM; i++)
    {
        if(frame_arr[i].status == ALLOCATED)
        {
            int cnt = 0;
            int expected_cnt = pow_2(frame_arr[i].level);
#ifdef DEBUG
            int level = frame_arr[i].level;
#endif
            printf("\r\n[SYSTEM INFO] Allocated Frame: "); printf_int(frame_arr[i].index); printf(", Level: "); printf_int(frame_arr[i].level);
            struct frame_t* curr = &frame_arr[i];
            while(curr){
                cnt++;
                i++;
                curr = curr->next;
            }
#ifdef DEBUG
            printf("\r\n[DEBUG] Allocated Frame Count: "); printf_int(cnt); printf(", Expected Count: "); printf_int(expected_cnt); 
            printf(" , level: "); printf_int(level);
#endif
            assert(cnt == expected_cnt, "Allocated Frame Count Error!");
            i--;
        }
    }
    printf("\n===============================");
}

static uint64_t round_size(uint64_t size) // return size to the nearest power of 2
{
    uint64_t ret = 1;
    while(ret < size) ret <<= 1;
    return ret;
}

static uint8_t get_level(uint32_t size)
{
    uint8_t level = 0;
    for(int i=FRAME_SIZE; i<size; i<<=1) level++;
    return level;
}

void* balloc(uint64_t size)
{
    uint32_t r_size = round_size(size);
    uint8_t level = get_level(r_size);
#ifdef DEBUG
    printf("\r\n[SYSTEM INFO] Round Size: "); printf_int(r_size); printf(", Level: "); printf_int(level);
#endif
    void* ret = NULL;

    for(int i=level; i<=MAX_LEVEL; i++)
    {
        struct frame_t* frame = flist_arr[i].head;
        if(frame != NULL)
        {
            uint8_t from_level = i;
            int start_index = frame->index;
#ifdef DEBUG
            printf("\r\n[DEBUG] From Level: "); printf_int(from_level);
            printf("\r\n[DEBUG] Start Index: "); printf_int(start_index);
#endif
            remove_from_flist(&flist_arr[from_level], frame);
            
            while(from_level > level) // e.g. get level 4 frame, but only have level 6 frame
            {
                from_level--;
                uint64_t level_size = pow_2(from_level);
                uint32_t split_index = start_index + level_size;

                frame_arr[split_index].level = from_level;
                frame_arr[split_index + level_size - 1].next = NULL;
                
                frame_arr[split_index].status = FREE;
                // printf("\r\n[SYSTEM INFO] Split Frame: "); printf_int(split_index); printf(", Level: "); printf_int(from_level);
                add_to_flist(&flist_arr[from_level], &frame_arr[split_index]);
            }
            frame->status = ALLOCATED;
            frame->level = level;
            frame_arr[start_index + pow_2(level) - 1].next = NULL;
#ifdef DEBUG
            printf("\r\n[SYSTEM INFO] Allocate Frame: "); printf_int(frame->index);
#endif
            ret = (void*)(uint64_t)(MALLOC_START_ADDR + start_index * FRAME_SIZE);
            break;
        }
    }
    if(ret == NULL)
    {
        printf("\r\n[SYSTEM ERROR] No Enough Memory!");
    }
#ifdef DEBUG
    print_flist(0, NULL);
    print_allocated(0, NULL);
#endif
    return ret;
}

static void* get_buddy_address(void* ptr, uint8_t level)
{
    return (void*)((uint64_t)ptr ^ (pow_2(level) * FRAME_SIZE));
}

static uint32_t get_buddy_index(void* ptr, uint8_t level)
{
    return ((uint64_t)get_buddy_address(ptr, level) - MALLOC_START_ADDR) >> 12;
}

int bfree(void* ptr)
{
    uint8_t max_level = buddy_system_list[get_bid((uint64_t)ptr)].max_level;
    uint32_t offset = buddy_system_list[get_bid((uint64_t)ptr)].index;
    
    struct frame_t* curr_frame = &frame_arr[((uint64_t)ptr - MALLOC_START_ADDR) >> 12];

    switch(curr_frame->status)
    {
        case FREE:
#ifdef DEBUG
            printf("\r\n[SYSTEM ERROR] Frame Already Free!");
#endif
            return 1;
        case LARGE_CONTIGUOUS:
#ifdef DEBUG
            printf("\r\n[SYSTEM ERROR] Frame is belong to Large Contiguous Frame!");
#endif
            return 1;
        case RESERVED:
#ifdef DEBUG
            printf("\r\n[SYSTEM ERROR] Frame is Reserved!");
#endif
            return 1;
        default:
            break;
    }

    // printf("\r\n[SYSTEM INFO] Free Frame Address: "); printf_hex((uint64_t)ptr);
    // buddy address = curr address ^ (block size)
    struct frame_t* buddy_frame = &frame_arr[get_buddy_index((ptr-offset*FRAME_SIZE), curr_frame->level)+offset];
#ifdef DEBUG
    printf("\r\n[SYSTEM INFO] Free Frame: "); printf_int(curr_frame->index); printf(", Level: "); printf_int(curr_frame->level);
    printf("\r\n[SYSTEM INFO] Buddy Frame: "); printf_int(buddy_frame->index); printf(", Level: "); printf_int(buddy_frame->level);
#endif
    if(buddy_frame->status != FREE)
    {
#ifdef DEBUG
        printf("\r\n[SYSTEM INFO] Buddy Frame is not FREE!");
#endif
        curr_frame->status = FREE;
    }
    else
    {
#ifdef DEBUG
        printf("\r\n[SYSTEM INFO] Buddy Frame is Free!");
#endif
    }
    while(buddy_frame->status == FREE && curr_frame->level < max_level)
    {
        curr_frame->status = FREE;
#ifdef DEBUG
        printf("\r\n[SYSTEM INFO] Merge Frame:"); printf_int(curr_frame->index); printf(" and Frame: "); printf_int(buddy_frame->index);
        printf(" to Level: "); printf_int(curr_frame->level + 1);
#endif

#ifdef DEBUG
        printf("\r\n[DEBUG] remove_from_flist: level: "); printf_int(buddy_frame->level); printf(" index: "); printf_int(buddy_frame->index);
#endif
        remove_from_flist(&flist_arr[buddy_frame->level], buddy_frame);
        
        uint32_t merge_index;
        if(buddy_frame->index < curr_frame->index)
        {
            merge_index = buddy_frame->index;
            frame_arr[buddy_frame->index + pow_2(buddy_frame->level) - 1].next = curr_frame;
            curr_frame->status = LARGE_CONTIGUOUS;
        }
        else
        {
            merge_index = curr_frame->index;
            frame_arr[curr_frame->index + pow_2(curr_frame->level) - 1].next = buddy_frame;
            buddy_frame->status = LARGE_CONTIGUOUS;
        }

#ifdef DEBUG
        printf(" with head frame: "); printf_int(merge_index);
#endif

        curr_frame = &frame_arr[merge_index];
        curr_frame->level++;
        if(curr_frame->level == max_level)
        {
            break;
        }
        buddy_frame = &frame_arr[get_buddy_index((void*)(uint64_t)(MALLOC_START_ADDR + (merge_index-offset) * FRAME_SIZE), curr_frame->level) + offset];
#ifdef DEBUG
        printf("\r\n[SYSTEM INFO] New Frame: "); printf_int(curr_frame->index); printf(", Level: "); printf_int(curr_frame->level);
        printf("\r\n[SYSTEM INFO] New Buddy Frame: "); printf_int(buddy_frame->index); printf(", Level: "); printf_int(buddy_frame->level);
#endif
    }
#ifdef DEBUG
    printf("\r\n[SYSTEM INFO] Add Frame:"); printf_int(curr_frame->index); printf(" to Level: "); printf_int(curr_frame->level);
#endif
    add_to_flist(&flist_arr[curr_frame->level], curr_frame);
    return 0;
}

void bfree_wrapper(int argc, char *argv[])
{
    if(argc != 2){
        printf("\nUsage: test_bfree <index>");
        return;
    }
    uint64_t index = atoi(argv[1]);
    if(index >= FRAME_NUM){
        printf("\nIndex out of range");
        return;
    }
    if(!bfree((void*)(MALLOC_START_ADDR + index * FRAME_SIZE)))
    {
        print_flist(0, NULL);
        print_allocated(0, NULL);
    }
}

void memory_pool_init()
{
    for(int i=0; i<MEMORY_POOL_SIZE; i++)
    {
        memory_pools[i].frame_used = 0;
        memory_pools[i].chunk_used = 0;
        memory_pools[i].size = 0;
        memory_pools[i].chunk_per_frame = 0;
        memory_pools[i].chunk_offset = 0;
        memory_pools[i].free_chunk = NULL;
        for(int j=0; j<MEMORY_POOL_DEPTH; j++)
        {
            memory_pools[i].frame_base_addrs[j] = NULL;
        }
    }
}

static void init_memory_pool_with_size(struct memory_pool_t* memory_pool, uint64_t size)
{
    memory_pool->size = size;
    memory_pool->chunk_per_frame = FRAME_SIZE / size;
}

static int get_memory_pool_id(uint64_t size)
{
    uint64_t r_size = round_size(size);
    
    if(r_size >= FRAME_SIZE) return -1; // larger than frame size, use general balloc

    for(int i=0; i<MEMORY_POOL_SIZE; i++)
    {
        if(memory_pools[i].size >= r_size)
        {
            return i;
        }
        else if(memory_pools[i].size == 0)
        {
#ifdef DEBUG
            printf("\r\n[DEBUG] Init New Memory Pool with chunk size: "); printf_int(r_size);
#endif
            init_memory_pool_with_size(&memory_pools[i], r_size);
            return i;
        }
    }
    return -2;
}

void* dynamic_alloc(uint64_t size)
{
    
    int ret = get_memory_pool_id(size);
    switch(ret)
    {
        case -1: 
            printf("\r\n[SYSTEM ERROR] Larger than Frame Size! Please use General balloc!");
            return 0;
        case -2:
            printf("\r\n[SYSTEM ERROR] No Enough Memory Pool! Please use General balloc!");
            return 0;
        default:
            break;
    }

    struct memory_pool_t* memory_pool = &memory_pools[ret];

    void* res = NULL;

    if(memory_pool->free_chunk != NULL) 
    {
        res = memory_pool->free_chunk;
        memory_pool->free_chunk = memory_pool->free_chunk->next; // point to the next free chunk
        return res;
    }

    if(memory_pool->frame_used >= MEMORY_POOL_DEPTH * memory_pool->chunk_per_frame)
    {
        printf("\r\n[SYSTEM ERROR] No Enough Memory Pool Frame!");
        return res;
    }

    if(memory_pool->chunk_used >= memory_pool->frame_used * memory_pool->chunk_per_frame)
    {
        if(memory_pool->frame_used == 0)
        {
            printf("\r\n[SYSTEM INFO] Create First Frame for Memory Pool with size: ");printf_int(memory_pool->size);
        }
        else
        {
            printf("\r\n[SYSTEM INFO] The Frame of Memory Pool with size: ");printf_int(memory_pool->size);printf(" is Full! ");
            printf("Create a New Frame!");
        }
            
        void* new_frame = balloc(FRAME_SIZE);
        memory_pool->frame_base_addrs[memory_pool->frame_used] = new_frame;
        memory_pool->frame_used++;
        memory_pool->chunk_offset = 0;
    }

    printf("\r\n[SYSTEM INFO] Allocate Chunk in Memory Pool with size: "); printf_int(memory_pool->size);
    // printf(" in Frame: "); printf_int(((struct frame_t*)memory_pool->frame_base_addrs[memory_pool->frame_used - 1])->index);

    res = memory_pool->frame_base_addrs[memory_pool->frame_used - 1] + memory_pool->chunk_offset * memory_pool->size;
    printf("\r\n[SYSTEM INFO] Allocate Chunk at address: "); printf_hex((uint64_t)res);
    memory_pool->chunk_offset++;
    memory_pool->chunk_used++;

    return res;
}

int dfree(void* ptr)
{
    int memory_pool_index = -1;
    for(int i=0; i<MEMORY_POOL_SIZE; i++)
    {
        for(int j=0; j<memory_pools[i].frame_used; j++)
        {
            void* prefix_addr = (void*)((uint64_t)ptr & 0xfffff000);
            if(memory_pools[i].frame_base_addrs[j] == prefix_addr)
            {
                memory_pool_index = i;
                break;
            }
        }
    }

    if(memory_pool_index == -1)
    {
        printf("\r\n[SYSTEM ERROR] Memory Pool Not Found!");
        return 1;
    }

    uint32_t frame_index = ((uint64_t)ptr - MALLOC_START_ADDR) >>12; // 2^12 = 4096 = FRAME_SIZE
    struct memory_pool_t* memory_pool = &memory_pools[memory_pool_index];

    // check whether the chunk is already in the free chunk list
    struct chunk_t* head_free_chunk = memory_pool->free_chunk;
    while(head_free_chunk)
    {
        if(head_free_chunk == ptr)
        {
            printf("\r\n[SYSTEM ERROR] Chunk Already Free!");
            return 1;
        }
        head_free_chunk = head_free_chunk->next;
    }

    printf("\r\n[SYSTEM INFO] Free Chunk in Frame: "); printf_int(frame_index);
    memory_pool->chunk_used--;
    struct chunk_t* last_free_chunk = memory_pool->free_chunk;
    memory_pool->free_chunk = (struct chunk_t*)ptr;
    memory_pool->free_chunk->next = last_free_chunk;

    printf("\r\n[SYSTEM INFO] Free Chunk at address: "); printf_hex((uint64_t)ptr); 
    return 0;
}