#ifndef _BUDDY_SYSTEM_2_
#define _BUDDY_SYSTEM_2_

#include <stdint.h>

#define FREE_MEM_BASE_ADDR 0x0
#define FREE_MEM_END_ADDR 0x3C000000

// Each frame is 4KB.
#define FRAME_SIZE 4096

// List stores page frames of size 4KB, 8KB, 16KB, 32KB, 64KB, 128KB, 256KB.
#define FREE_LIST_SIZE 7

// Maximum allocation from the buddy system permitted.
#define MAX_ALLOC_CNT 10

// Status of the frame.
// FREE_FRAME -> Frame is free but belong to a larger contiguous page.
#define FREE_FRAME 10
// USED_FRAME -> Allocated frame. Unusable.
#define USED_FRAME 11

// 'A' -> 101, 'B' -> 102, 'C' -> 103
//  At most 64 frames will be allocated(256 KB)
#define SLOT_64B 'A'
#define SLOT_128B 'B'
#define SLOT_256B 'C'
#define SLOT_NOT_ALLOCATED 'D'

// Reserve memory


typedef struct entry {
    short status;
    int ind;
    struct entry* next;
    struct entry* prev;

    // Linked list for dynamic allocator.
    // Storing allocated slots.
    // Ex: slot 0 is used -> FFFF FFFF FFFF FFF0(In hexadecimal)
    uint64_t bitmap;
    // Determine the slot size.
    char slot_size;
    // Remaining slot count.
    short slot_cnt;
    struct entry* next_slot;
    struct entry* prev_slot;
} entry_t;

typedef struct list_header {
    // Points to head entry and tail entry of the list.
    entry_t* head;
    entry_t* tail;
} list_header_t;

typedef struct alloc_header {
    // Points to head entry and tail entry of the list.
    entry_t* head;
    entry_t* tail;
    // Size of allocated block.
    short status;
} alloc_header_t;

extern alloc_header_t* alloc_frame_list[MAX_ALLOC_CNT];
extern entry_t** frame_arr;

void memory_reserve(uint64_t start, uint64_t end);
void insert_freelist(int start_frame, int contiguous_frames);
void init_buddy_system(void);
int alloc_page(uint64_t request_size);
void free_page(int start_frame_ind);
void show_free_list(void);
void show_alloc_list(void);
int find_buddy(int ind, int exp);

#endif