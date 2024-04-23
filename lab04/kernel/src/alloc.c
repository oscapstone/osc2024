#include "alloc.h"
#include "mem.h"
#include "io.h"
#include "lib.h"

#define UNUSED(x) (void)(x)

static char* HEAP_HEADER;
static char* HEAP_REAR;

void  mem_init()
{
    HEAP_HEADER = (char*)&_end;
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


static struct frame_t frame_arr[FRAME_NUM];
static struct flist_t flist_arr[MAX_LEVEL + 1];

#define FREE                 0
#define ALLOCATED            1
#define LARGE_CONTIGUOUS     2
#define RESERVED             3


static int get_next_Free_frame_index(uint8_t level)
{
    struct frame_t* frame = flist_arr[level].head->next;
    while(frame != NULL)
    {
        if(frame->status == FREE)
        {
            return frame->index;
        }
        frame = frame->next;
    }
    return -1;
}


static void add_to_flist(struct flist_t* flist, struct frame_t* frame)
{
    struct frame_t *curr = flist->head;
    while(curr) curr = curr->next;
    if(curr) curr->next = frame;
    else 
    {
        // printf("\r\n[SYSTEM INFO] Add Frame: "); printf_int(frame->index); printf(" to Level: "); printf_int(frame->level);
        flist->head = frame;
        // printf("\r\n[SYSTEM INFO] Frame status: "); printf_int(frame->status);
    }
}

static void remove_from_flist(struct flist_t* flist, struct frame_t* frame)
{
    uint8_t level = frame->level;
    struct frame_t *curr = flist->head;
    struct frame_t *prev = NULL;
    int next_index = get_next_Free_frame_index(level);
    while(curr)
    {
        if(curr == frame)
        {
            if(prev) prev->next = next_index == -1 ? NULL : &frame_arr[next_index];
            else flist->head = next_index == -1 ? NULL : &frame_arr[next_index];
#ifdef DEBUG
            printf("\r\n[DEBUG] Remove Frame: "); printf_int(frame->index); printf(" from flist Level: "); printf_int(frame->level);
#endif
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

void frame_init()
{
    printf("\r\n[SYSTEM INFO] Frame Init Start at: ");
    printf_hex(MALLOC_START_ADDR);
    printf(", End at: ");
    printf_hex(MALLOC_END_ADDR);
    printf(", Total Size: ");
    printf_hex(MALLOC_TOTAL_SIZE);
    printf(", Frame Num: ");
    printf_hex(FRAME_NUM);

    printf("\r\n[SYSTEM INFO] Max Contiguous Size: ");
    printf_hex(pow(2, MAX_LEVEL) * FRAME_SIZE);


    for(int i=0; i<=MAX_LEVEL + 1; i++)
    {
        flist_arr[i].head = NULL;
    }

    for(int i=0; i<FRAME_NUM; i++)
    {
        frame_arr[i].index = i;

        if(i == 0)
        {
            frame_arr[i].status = FREE;
            frame_arr[i].level = MAX_LEVEL;
            frame_arr[i].prev = NULL;
            add_to_flist(&flist_arr[MAX_LEVEL], &frame_arr[i]);
        }
        else
        {
            frame_arr[i].status = LARGE_CONTIGUOUS;
            frame_arr[i].level = 0;
            frame_arr[i].prev = &frame_arr[i-1];
            frame_arr[i-1].next = &frame_arr[i];
        }
    }
    frame_arr[FRAME_NUM-1].next = NULL;
}

void print_flist(int argc, char* argv[])
{
    printf("\r\n========== free list ==========");
    for(int i=0; i<=MAX_LEVEL; i++)
    {
        struct frame_t* frame = flist_arr[i].head;
        printf("\r\n[SYSTEM INFO] Level: "); printf_int(i); printf(", Frame: ");
        int sign = 0;
        while(frame != NULL)
        {
            // printf("\r\n[SYSTEM INFO] Frame: "); printf_int(frame->index); printf(", Status: "); printf_int(frame->status);
            if(frame->status == FREE)
            {
                if(sign) printf("-> ");
                printf_int(frame->index);
                sign = 1;
            }
            frame = frame->next;
        }
    }
}

void print_allocated(int argc, char* argv[])
{
    printf("\r\n=========== allocated ===========");
    for(int i=0; i<FRAME_NUM; i++)
    {
        if(frame_arr[i].status == ALLOCATED)
        {
            int cnt = 0;
            int expected_cnt = pow(2, frame_arr[i].level);
            printf("\r\n[SYSTEM INFO] Allocated Frame: "); printf_int(frame_arr[i].index); printf(", Level: "); printf_int(frame_arr[i].level);
            struct frame_t* curr = &frame_arr[i];
            while(curr){
                cnt++;
                i++;
                curr = curr->next;
            }
            assert(cnt == expected_cnt, "Allocated Frame Count Error!");
            i--;
        }
    }
}

static uint64_t* round_size(uint64_t size) // return size, level
{
    uint64_t *ret = (uint64_t*)simple_malloc(sizeof(uint64_t) * 2);
    ret[0] = 1; ret[1] = 0;
    
    while(ret[0] < size)
    {
        ret[0] *= 2;
        ret[1]++;
    }
    
    return ret;
}

void* balloc(uint64_t size)
{
    uint64_t* round_info = round_size(size);
    uint64_t round_size = round_info[0];
    uint8_t level = (uint8_t)round_info[1];
    printf("\r\n[SYSTEM INFO] Round Size: "); printf_int(round_size); printf(", Level: "); printf_int(level);
    
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
            uint32_t free_index = get_next_Free_frame_index(from_level);
            flist_arr[from_level].head = free_index != -1 ? &frame_arr[free_index] : NULL;
            
            while(from_level > level) // e.g. get level 4 frame, but only have level 6 frame
            {
                from_level--;
                uint64_t level_size = pow(2, from_level);
                uint32_t split_index = start_index + level_size;

                frame_arr[split_index].level = from_level;
                frame_arr[split_index + level_size - 1].next = NULL;
                
                frame_arr[split_index].status = FREE;
                // printf("\r\n[SYSTEM INFO] Split Frame: "); printf_int(split_index); printf(", Level: "); printf_int(from_level);
                add_to_flist(&flist_arr[from_level], &frame_arr[split_index]);
            }
            frame->status = ALLOCATED;
            frame->level = level;
            frame_arr[start_index + round_size - 1].next = NULL;
            printf("\r\n[SYSTEM INFO] Allocate Frame: "); printf_int(frame->index);
            ret = (void*)(uint64_t)(MALLOC_START_ADDR + start_index * FRAME_SIZE);
            break;
        }
    }
    if(ret == NULL)
    {
        printf("\r\n[SYSTEM ERROR] No Enough Memory!");
    }
    print_flist(0, NULL);
    print_allocated(0, NULL);
    return ret;
}

static void* get_buddy_address(void* ptr, uint8_t level)
{
    return (void*)((uint64_t)ptr ^ (pow(2, level) * FRAME_SIZE));
}

static uint32_t get_buddy_index(void* ptr, uint8_t level)
{
    return ((uint64_t)get_buddy_address(ptr, level) - MALLOC_START_ADDR) / FRAME_SIZE;
}

void bfree(void* ptr)
{
    struct frame_t* curr_frame = &frame_arr[((uint64_t)ptr - MALLOC_START_ADDR) / FRAME_SIZE];
    
    switch(curr_frame->status)
    {
        case FREE:
            printf("\r\n[SYSTEM ERROR] Frame Already Free!");
            return;
        case LARGE_CONTIGUOUS:
            printf("\r\n[SYSTEM ERROR] Frame is belong to Large Contiguous Frame!");
        default:
            break;
    }

    // buddy address = curr address ^ (block size)
    struct frame_t* buddy_frame = &frame_arr[get_buddy_index(ptr, curr_frame->level)];
    
    printf("\r\n[SYSTEM INFO] Free Frame: "); printf_int(curr_frame->index); printf(", Level: "); printf_int(curr_frame->level);
    printf("\r\n[SYSTEM INFO] Buddy Frame: "); printf_int(buddy_frame->index); printf(", Level: "); printf_int(buddy_frame->level);
    if(buddy_frame->status == ALLOCATED || buddy_frame->status == RESERVED)
    {
        printf("\r\n[SYSTEM INFO] Buddy Frame is Allocated or Reserved!");
        curr_frame->status = FREE;
    }
    else
    {
        printf("\r\n[SYSTEM INFO] Buddy Frame is Free!");
    }
    while(buddy_frame->status == FREE && curr_frame->level < MAX_LEVEL)
    {
        curr_frame->status = FREE;
        printf("\r\n[SYSTEM INFO] Merge Frame:"); printf_int(curr_frame->index); printf(" and Frame: "); printf_int(buddy_frame->index);
        printf(" to Level: "); printf_int(curr_frame->level + 1);
        uint32_t merge_index;
        if(buddy_frame->index < curr_frame->index)
        {
            merge_index = buddy_frame->index;
            frame_arr[buddy_frame->index + pow(2, buddy_frame->level) - 1].next = curr_frame;
            curr_frame->status = LARGE_CONTIGUOUS;
        }
        else
        {
            merge_index = curr_frame->index;
            frame_arr[curr_frame->index + pow(2, curr_frame->level) - 1].next = buddy_frame;
            buddy_frame->status = LARGE_CONTIGUOUS;
        }
#ifdef DEBUG
        printf("\r\n[DEBUG] remove_from_flist: level: "); printf_int(buddy_frame->level); printf(" index: "); printf_int(buddy_frame->index);
#endif
        remove_from_flist(&flist_arr[buddy_frame->level], buddy_frame);

        printf(" with head frame: "); printf_int(merge_index);

        curr_frame = &frame_arr[merge_index];
        curr_frame->level++;
        if(curr_frame->level == MAX_LEVEL)
        {
            break;
        }
        buddy_frame = &frame_arr[get_buddy_index((void*)(uint64_t)(MALLOC_START_ADDR + merge_index * FRAME_SIZE), curr_frame->level)];

        printf("\r\n[SYSTEM INFO] New Frame: "); printf_int(curr_frame->index); printf(", Level: "); printf_int(curr_frame->level);
        printf("\r\n[SYSTEM INFO] New Buddy Frame: "); printf_int(buddy_frame->index); printf(", Level: "); printf_int(buddy_frame->level);
    }
    printf("\r\n[SYSTEM INFO] Add Frame:"); printf_int(curr_frame->index); printf(" to Level: "); printf_int(curr_frame->level);
    
    add_to_flist(&flist_arr[curr_frame->level], curr_frame);
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
    bfree((void*)(MALLOC_START_ADDR + index * FRAME_SIZE));
    print_flist(0, NULL);
    print_allocated(0, NULL);
    printf("\n===============================");
}