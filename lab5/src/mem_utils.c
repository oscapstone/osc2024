#include "../include/mem_utils.h"
#include "../include/list.h"
#include "../include/mini_uart.h"
#include <stdint.h>

/* for simple malloc */
#define HEAP_SIZE   0x1000000

/* for frame array */
#define START_ADDR      0x00
#define END_ADDR        0x3C000000
#define ALLOCABLE_SPACE (END_ADDR - START_ADDR)
#define PAGE_SIZE       0x1000
#define NUM_ENTRIES     (ALLOCABLE_SPACE / PAGE_SIZE)
#define FREE            255
#define UNALLOCABLE     254
#define RESERVED        253

/* for accessing heap_start */
extern char _end;
extern char *cpio_addr;
extern char *cpio_addr_end;
extern char *dtb_start;
extern char *dtb_end;

// static char HEAP_USE[HEAP_SIZE] = {0};
// static unsigned int HEAP_OFFSET = 0;
// static unsigned int MEM_START = 0x10000000;
// static unsigned int MEM_END = 0x20000000;

/* define the struct of frame_array_element: it will be inserted in the free list during page allocation and inserted in the list of dynamic allocation */
typedef struct __attribute__((aligned(16))) frame_array_element {
    struct list_head head;         // for linked in free list or in dynamic allocation list.
    uint32_t contiguous_pages;     // this value is used for dynamic memory allocator. positive number: start of contiguous page, others: 0.
    uint32_t size;                 // this value is used for recording the chunk size it allocated.
    char usage;                    // val: 4K * (2^val), 255: free, 254: not allocable.
} frame_array_t;

/* define the struct of each free list: it contains head of linked list */
typedef struct __attribute__((aligned(16))) free_list_node {
    struct list_head head;         // head of the doubly linked list of free list of specific order.
} free_list_t;

/* define the struct of each chunk list: it contains head of doubly linked list and chunk_size */
typedef struct __attribute__((aligned(16))) chunk_list_node {
    struct list_head head;
    uint32_t chunk_size;
} chunk_list_t;

/* global variable: array_start, freelist_start */
frame_array_t *array_start;
free_list_t *freelist_start;
chunk_list_t *chunk_list_start;

/* some helper function that is only used in this compile unit */
static uint32_t find_biggest_order(uint32_t number)
{
    uint32_t i;
    for (i = 0; i <= 31; i++) {
        uint32_t mask = 1 << i;
        if (mask >= number)
            break;
    }
    return i;
}

static uint32_t pow2(uint32_t order)
{
    uint32_t result = 1;
    return (result << order);
}

static void show_list(struct list_head *head, uint32_t order)
{
    printf("=========================================\n");
    printf("The order %d free list content:\n", order);
    for (struct list_head *curr = head->next; curr != head; curr = curr->next) 
        printf("array index : %d\n", (uint32_t)((void *)curr - (void *)array_start) / sizeof(frame_array_t));
    printf("=========================================\n");
}

static void show_entire_list(uint32_t biggest_order)
{
    for (int i = 0; i <= biggest_order; i++) {
        show_list(&freelist_start[i].head, i);
    }
}

static uint32_t multiple_of_four(const uint32_t size)
{    
    return ((size + 3) >> 2) << 2;
}

static uint32_t nearest_order(const uint32_t size)
{
    /* find the order that is bigger or equal to the size. */
    uint32_t multiple = size / 4;
    uint32_t i;
    for (i = 0; i <= 31; i++) {
        uint32_t mask = 1 << i;
        if (mask >= multiple)
            break;
    }
    return i;
}

static uint32_t get_frame_index(struct list_head *head, frame_array_t *array_zero, uint32_t frame_ele_size)
{
    /* used to get the frame index of the allocated address (from the freelist) */
    return ((uint32_t)((uint32_t)head - (uint32_t)array_zero) / frame_ele_size);
}

static uint32_t get_half_index(uint32_t frame_index, uint32_t order)
{
    /* used to get the frame index of the half part of contiguous memory */
    return frame_index + (1 << order);
}

static uint32_t count_frame_index(char *addr)
{
    /* used to count the frame index from memory address */
    return (uint32_t)(((uint32_t)(addr) - START_ADDR) / PAGE_SIZE);
}

static void page_allocate_adjust(uint32_t frame_index, uint32_t find_order, uint32_t order)
{
    /* this function is used to modify the free list from "find_order" to "order". */
    for (int i = find_order; i > (int)order; i--) {
        /* partition the contiguous memory into two and reserve the last half part into lower list */
        // printf("Now i = %d, and order = %d\n", i, order);
        /* a. get the start address of the first half part */
        char *start_address = (char *)(START_ADDR + (PAGE_SIZE * frame_index));
        // printf("First part start address: 0x%8x\n", start_address);
        
        /* b. get the start address of the last half part */
        uint32_t half_index = get_half_index(frame_index, i - 1);
        // printf("Half frame index: %d\n", half_index);
        char *half_address = (char *)(START_ADDR + (PAGE_SIZE * half_index));
        // printf("Second part start_address: 0x%8x\n", half_address);
        
        /* c. put "half_index" in the free list of next order. */
        list_add(&array_start[half_index].head, &freelist_start[i-1].head, freelist_start[i-1].head.next);

        /* d. adjust the information in the first and last half part of frame array */
        array_start[frame_index].usage = i - 1;
        array_start[half_index].usage = i - 1;
    }
}

static uint32_t find_buddy(uint32_t array_index, uint32_t order)
{
    /* return the buddy of array_index in that order */
    return array_index ^ (1 << order);
}

static uint32_t min_index(uint32_t a, uint32_t b)
{
    /* used to find buddy owner */
    return (a < b)? a: b;
}

static uint32_t max_index(uint32_t a, uint32_t b)
{
    /* used to return bigger buddy */
    return (a > b)? a: b;
}

static void merge_iteratively(uint32_t array_index)
{
    /* for each frame array index, merge to the current max order it can be */
    uint32_t max_order = find_biggest_order(NUM_ENTRIES);
    uint32_t start_index = array_index;
    for (uint32_t i = 0; i < max_order; i++) {
        /* 1. find buddy of array_index and check the "usage": 
              the same: change the "usage" of smaller index into next order; and "usage" of bigger index into FREE */
        uint32_t buddy = find_buddy(start_index, i);
        if ((array_start[buddy].usage == array_start[start_index].usage) && (array_start[buddy].usage < max_order)) {
            uint32_t buddy_owner = min_index(buddy, start_index);
            uint32_t bigger_buddy = max_index(buddy, start_index);
            array_start[buddy_owner].usage = i + 1;
            array_start[bigger_buddy].usage = FREE;
            list_del(&array_start[buddy_owner].head);
            INIT_LIST_HEAD(&array_start[buddy_owner].head);
            list_del(&array_start[bigger_buddy].head);
            INIT_LIST_HEAD(&array_start[bigger_buddy].head);

            list_add(&array_start[buddy_owner].head, &freelist_start[i+1].head, freelist_start[i+1].head.next);

            start_index = buddy_owner;
        } else {
            break;
        }
    }
}

static void bottom_up_merge(uint32_t array_index, uint32_t size)
{
    /* new version: since function "merge_iteratively" will merge current node to the max order it can be, no more check needed. 
       caution: starts merging from order 0 */
    for (uint32_t i = array_index; i < (array_index + size); i++)
        merge_iteratively(i);
}

static uint32_t count_reserved_area(uint32_t index, uint32_t maximum_index)
{
    uint32_t now_index = index; 
    while (now_index < maximum_index) {
        if (array_start[now_index].usage == RESERVED)
            now_index++;
        else
            break;
    }
    return now_index - index;
}

static uint32_t find_chunk_index(uint32_t size)
{
    for (int i = 0; i < 9; i++) {
        if (16 * pow2(i) >= size)
            return i;
    }
}

static void create_memory_pool(uint32_t size)
{
    /* 1. allocate a page */
    char *page_start = (char *)page_frame_allocate(4);       // 4 means 4 KB

    /* 2. */
}
// static void create_memory_pool(uint32_t size)
// {
//     /* 1. allocate a page */
//     char *new_page = (char *)page_frame_allocate(4);       // 4 means 4 KB

//     /* 2. find its frame index, remove from freelist, and put it into d_freelist */
//     uint32_t frame_index = count_frame_index(new_page);
//     list_del(&array_start[frame_index].head);
//     list_add(&array_start[frame_index].head, &d_freelist_start[0].head, d_freelist_start[0].head.next);
//     show_d_freelist(&d_freelist_start[0].head);

//     /* 3. modify "max_chunks" and "now_chunks" for frame_index */
//     uint32_t page_size = PAGE_SIZE;
//     uint32_t num_chunks = page_size / size;
//     array_start[frame_index].max_chunks = num_chunks;
//     array_start[frame_index].now_chunks = num_chunks;

//     /* 4. using for loop to link all free chunks */
//     uint32_t page_start = (uint32_t)new_page;
//     for (uint32_t i = page_start; i < (page_start + page_size); i += size) {
//         struct list_head *tmp_head = (struct list_head *)i;
//         printf("address: %8x\n", (char *)i);
//         // list_add(tmp_head, &array_start[frame_index].d_head, array_start[frame_index].d_head.next);
//     }
// }

// static void *dalloc_helper(uint32_t size)
// {
//     void *free_space = NULL;
//     struct list_head *head = &d_freelist_start[0].head;
//     for (struct list_head *curr = head->next; curr != head; curr = curr->next) {
//         if (((frame_array_t *)curr)->now_chunks > 0) {
//             free_space = ((frame_array_t *)curr)->d_head.next;
//             list_del(((frame_array_t *)curr)->d_head.next);
//             ((frame_array_t *)curr)->now_chunks--;
//             break;
//         }
//     }
//     return free_space;
// }


/* functions that are used for buddy system, dynamic memory allocator, and startup allocator */
char *mem_align(char *addr, unsigned int number)
{
    /* this function is used to align the memory address to the nearest power of 2 */
    uint64_t x = (uint64_t) addr;
    uint64_t mask = number - 1;
    return (char *)((x + (mask)) & (~mask));
}

void *malloc(unsigned int size)
{    
    /* first, preserve 256 bytes for printf buffer */
    static char *HEAP_ADDR = &_end + 256;

    /* align 16 bytes */
    HEAP_ADDR = mem_align(HEAP_ADDR, 16);
    
    /* then, calculate the heap address is legal or not */
    if ((HEAP_ADDR - &_end) + size >= HEAP_SIZE)
        return NULL;
    
    char *now_addr = HEAP_ADDR;
    HEAP_ADDR += size;
    // printf("Now heap addr: %8x\n", HEAP_ADDR);                 // try to print current heap address
    return now_addr;
}

void *show_heap_end(void)
{
    /* a helper function that reveals the end of legal heap address */
    return (&_end + HEAP_SIZE);
}

static frame_array_t *frame_array_init(void)
{
    /* print the sizeof frame_array_t and observe the struct packing */
    // printf("sizeof(frame_array_t): %d\n", sizeof(frame_array_t));
    // printf("sizeof(struct list_head): %d\n", sizeof(struct list_head));
    
    /* allocation of memory for frame array */
    uint32_t num_entries = NUM_ENTRIES;
    array_start = (frame_array_t *)malloc(sizeof(frame_array_t) * NUM_ENTRIES);
    // printf("frame_array start: %8x\n", array_start);
    // printf("Number of entries: %8d\n", num_entries);
    // printf("frame_array_end: %8x\n", array_start + NUM_ENTRIES);

    /* Initialize the value of each entry: all 0 */
    for (int i = 0; i < num_entries; i++) 
        array_start[i].usage = 0;

    // printf("array[0]: %d\n", array_start[0].usage);
    // printf("array[1]: %d\n", array_start[1].usage);
    
    /* return the start address of the frame array. */
    return array_start;
}

static free_list_t *freelist_init(void)
{
    /* allocation of memory for free list */
    uint32_t biggest_order = find_biggest_order(NUM_ENTRIES);
    // printf("sizeof(free_list_t): %d\n", sizeof(free_list_t));
    
    freelist_start = (free_list_t *)malloc(sizeof(free_list_t) * (biggest_order + 1));
    
    /* Initialize the free list of each order */
    for (int i = 0; i <= biggest_order; i++) {
        INIT_LIST_HEAD(&freelist_start[i].head);
    }

    /* put all the allocable index into order 0 */
    for (uint32_t i = 0; i < NUM_ENTRIES; i++) {
        if (array_start[i].usage == 0)
            list_add(&array_start[i].head, &freelist_start[0].head, freelist_start[0].head.next);
    }

    return freelist_start;
}

static chunk_list_t *chunk_list_init(void)
{
    /* allocation of memrory for chunk_list */
    uint32_t num_chunk_list = 9;              // 16, 32, 64, 128, 256, 512, 1024, 2048, 4096;
    // printf("sizeof(chunk_list_t): %d\n", sizeof(chunk_list_t));

    chunk_list_start = (chunk_list_t *)malloc(sizeof(chunk_list_t) * num_chunk_list);

    /* Initialize all the chunk_list */
    for (int i = 0; i < num_chunk_list; i++) {
        INIT_LIST_HEAD(&chunk_list_start[i].head);
        chunk_list_start[i].chunk_size = 16 * pow2(i);
        // printf("chunk size of chunk_list[%d]: %d\n", i, chunk_list_start[i].chunk_size);
    }

    return chunk_list_start;
}

void buddy_system_init(void)
{
    // /* first, get the start address of frame array */
    array_start = frame_array_init();

    /* get the biggest order of free list.
       caution: the index of free list is zero-ordered */
    uint32_t biggest_order = find_biggest_order(NUM_ENTRIES);
    // printf("NUMENTRIES: %d\n", NUM_ENTRIES);
    // printf("biggest order: %d\n", biggest_order);

    /* kernel size */
    char *kernel_start = 0x80000;
    char *kernel_end = (char *)(&_end + HEAP_SIZE);
    // printf("kernel_end :%8x\n", kernel_end);

    /* reserve area */
    memory_reserve((char *)0x0000, (char *)0x1000);
    memory_reserve(cpio_addr, cpio_addr_end);
    memory_reserve(dtb_start, dtb_end);
    memory_reserve(kernel_start, kernel_end);
    memory_reserve((char *)0x80000 - 2 * PAGE_SIZE, (char *)0x80000);

    /* after reserve some important area, we can init the freelist */
    freelist_start = freelist_init();
    
    /* In the beginning, set "usage" in every frame array element into 0, and then do the bottom up merge. */
    bottom_up_merge(0, (uint32_t)NUM_ENTRIES);

    /* print the log of each free list, which is helpful for debugging */
    // show_entire_list(biggest_order);
}

void *page_frame_allocate(uint32_t size)
{
    /* 1. align the size into multiple of 4KB */
    // printf("Before alignment: %d\n", size);
    size = multiple_of_four(size);
    // printf("After alignment: %d\n", size);

    /* 2. align the multiple of 4KB into the nearest power of 2 */
    uint32_t order = nearest_order(size);
    // printf("Nearest order: %d\n", order);

    /* 3. search the free list from the order until the maximum order */
    uint32_t max_order = find_biggest_order(NUM_ENTRIES);
    int32_t find_order = -1;
    uint32_t frame_index = -1;                    // actually 2^31 - 1; 
    for (int i = order; i <= max_order; i++) {
        /* Check the number of node in the list of that order -> 0: find the next order; others: take it.
           Furthermore, we need to delete the first node in the list and get its frame_index. */
        if (freelist_start[i].head.next != &freelist_start[i].head) {
            find_order = i;
            frame_index = get_frame_index(freelist_start[i].head.next, array_start, sizeof(frame_array_t));
            list_del(freelist_start[i].head.next);
            break;
        }
    }
    // show_entire_list(max_order);
    // printf("First frame index: %d\n", frame_index);
    // printf("Find order: %d\n", find_order);

    /* 4. adjust the content of free list in order from [find_order] to [order] */
    if (find_order != -1) {
        /* This function is used to modify the free list from "find_order" to "order". 
           If "find_order" == "order", then nothing happended. */
        page_allocate_adjust(frame_index, find_order, order);

        // i == order
        /* a. first, show all free list content */
        // show_entire_list(max_order);
        
        /* b. get the start address of the first half part */
        // printf("First frame index: %d\n", frame_index);
        char *start_address = (char *)(START_ADDR + (PAGE_SIZE * frame_index));
        // printf("First part start address: 0x%8x\n", start_address);

        /* c. check how many pages need to be allocated */
        uint32_t num_pages = size / 4;                   // 4 means 4KB
        // printf("Number of pages: %d\n", num_pages);

        /* d. adjust the information of allocated pages: 
              1. it may be used by the dynamic allocator, then change the first allocated frame array "contiguous_pages"
              2. other frame array "usage" would be "UNALLOCABLE" */
        uint32_t pages_of_order = pow2(order);
        // printf("Pages of order = %d\n", pages_of_order);
        for (uint32_t i = frame_index; i < (frame_index + num_pages); i++) {
            if (i == frame_index)
                array_start[i].contiguous_pages = num_pages;
            array_start[i].usage = UNALLOCABLE;
        }

        /* e. modify the information of rest of pages, and put it in the free list of order 0 */
        for (uint32_t i = (frame_index + num_pages); i < (frame_index + pages_of_order); i++) {
            array_start[i].usage = 0;
            INIT_LIST_HEAD(&array_start[i].head);
            list_add(&array_start[i].head, &freelist_start[0].head, freelist_start[0].head.next);
        }

        // /* f. bottom up merge the pages from "frame_index" (by its order), and check the frame array index in this range to decide how to add into free list. */
        bottom_up_merge(frame_index, pages_of_order);

        // /* g. show the entire list */
        // show_entire_list(max_order);

        // /* h. return the start_address */
        return start_address;

    } else {
        printf("There is no allocable memory!!!\n");
        return NULL;
    }

}

void page_frame_free(char *addr)
{
    /* 1. check the address and find its corresponding frame index */
    uint32_t frame_index = count_frame_index(addr);
    // printf("The start frame index: %d\n", frame_index);
    
    /* 2. check the "countiguous_pages" in this frame index */
    uint32_t free_pages = array_start[frame_index].contiguous_pages;
    // printf("%d pages that need to be free\n", free_pages);

    /* 3. modify the "usage" of pages that need to be free (set to zero), add to the free list of order 0 */
    array_start[frame_index].contiguous_pages = 0;
    for (uint32_t i = frame_index; i < (frame_index + free_pages); i++) {
        array_start[i].usage = 0;
        INIT_LIST_HEAD(&array_start[i].head);
        list_add(&array_start[i].head, &freelist_start[0].head, freelist_start[0].head.next);
    }

    /* 4. Using for loop to merge these pages iteratively */
    for (uint32_t i = frame_index; i < (frame_index + free_pages); i++) 
        merge_iteratively(i);

    /* 5. Show the result of frame array or free list */
    uint32_t max_order = find_biggest_order(NUM_ENTRIES);
    // show_entire_list(max_order);
}

void show_memory_layout()
{
    /* 1. first, count the maximum_index and max_order */
    uint32_t maximum_index = NUM_ENTRIES;
    uint32_t max_order = find_biggest_order(NUM_ENTRIES);

    /* 2. using while loop to traverse all the frame array index */
    uint32_t now_index = 0;
    while (now_index < maximum_index) {
        if (array_start[now_index].usage <= max_order) {
            uint32_t index = pow2(array_start[now_index].usage);
            char *now_addr = (char *)((uint32_t)(START_ADDR) + PAGE_SIZE * now_index);
            printf("%16x %8d KB\n", now_addr, index * 4);
            now_index += index;
        } else if (array_start[now_index].usage == RESERVED) {
            uint32_t diff = count_reserved_area(now_index, maximum_index);
            char *now_addr = (char *)((uint32_t)(START_ADDR) + PAGE_SIZE * now_index);
            printf("%16x %8d KB reserved\n", now_addr, diff * 4);
            now_index += diff;
        } else if (array_start[now_index].usage == UNALLOCABLE) {
            now_index++;
        } else {
            printf("ERROR!!!\n");
        }
    }
}

void memory_reserve(char *start, char *end)
{
    /* 1. first, get the frame array index of start and end */
    uint32_t start_index = count_frame_index(start);
    uint32_t end_index = count_frame_index(end);

    /* 2. using for loop to modify the "usage" from start_index to end_index */
    for (uint32_t i = start_index; i <= end_index; i++)
        array_start[i].usage = RESERVED;
}

void dynamic_allocator_init(void)
{
    /* Initialize the chun_list */
    chunk_list_start = chunk_list_init(); 
}
void *chunk_alloc(uint32_t size)
{
    /* 1. find the chunk_list index */
    uint32_t size_index = find_chunk_index(size);
    size = 16 * pow2(size_index);
    // printf("Corresponding size: %d\n", size);
    // printf("index of chunk list: %d\n", size_index);

    /* 2. check the chunk_list_start[size_index].head if it is empty */
    if (list_is_empty(&chunk_list_start[size_index].head)) {
        // printf("Need to allocate a new page\n");

        void *page_address = page_frame_allocate(4);
        uint32_t frame_index = count_frame_index(page_address);
        // printf("Allocate page number %d\n", frame_index);
        array_start[frame_index].size = size;

        for (uint32_t i = 0; i < PAGE_SIZE; i += size) {
            struct list_head *tmp = (struct list_head *)((char *)page_address + i);
            // printf("now_addr: %8x\n", tmp);
            list_add_tail(tmp, &chunk_list_start[size_index].head);
        }
    }

    struct list_head *return_addr = chunk_list_start[size_index].head.next;
    list_del(return_addr);

    return (void *)return_addr;
}

void chunk_free(char *addr)
{
    /* 1. search the page index */
    uint32_t page_index = count_frame_index(addr);
    // printf("address is in page number %d\n", page_index);

    /* 2. find the index of chunk list through "size" in frame array */
    uint32_t size = array_start[page_index].size;
    // printf("Corresponding size: %d\n", size);
    uint32_t size_index = find_chunk_index(size);

    /* 3. free the space into the chunk_list */
    list_add((struct list_head *)addr, &chunk_list_start[size_index].head, chunk_list_start[size_index].head.next);
}

// void dynamic_allocator_init(void)
// {
//     /* 1. prepare two freelist head: one for small allocate: [16, 256] bytes, another for bigger allocate */
//     d_freelist_start = (free_list_t *)malloc(sizeof(free_list_t) * 2);
//     for (int i = 0; i < 2; i++) {
//         INIT_LIST_HEAD(&d_freelist_start[i].head);
//     }
    
//     /* 2. [16, 256] bytes, allocate a page for each of size */
//     for (uint32_t i = 16; i <= 256; i *= 2) {
//         create_memory_pool(i);
//     }

//     /* 3. show d_freelist */
//     show_d_freelist(&d_freelist_start[0].head);
// }

// void *chunk_alloc(uint32_t size)
// {
//     void *free_space = dalloc_helper(size);
//     printf("chunk addr: %8x\n", free_space);
//     show_d_freelist(&d_freelist_start[0].head);
//     return free_space;
// }

// void chunk_free(char *addr)
// {
//     /* 1. calculate the frame index of addr */
//     uint32_t frame_index = ((uint32_t)addr) >> 12;            // 12 means PAGE_SHIFT
//     printf("The frame index: %d\n",frame_index);

//     /* 2. modify the :now_chunks* and insert the chunk into "d_head".next */
//     array_start[frame_index].now_chunks++;
//     list_add((struct list_head *)addr, &array_start[frame_index].d_head, array_start[frame_index].d_head.next);
// }

/*
 * with `-nostdlib` compiler flag, we have to implement this function since
 * compiler may generate references to this function
 */
void memset(void* b, int c, uint32_t len)
{
    uint32_t offset = mem_align(b, sizeof(void*)) - (char *)b;
    len -= offset;
    uint32_t word_size = len / sizeof(void*);
    uint32_t byte_size = len % sizeof(void*);
    unsigned char* offset_ptr = (unsigned char*)b;
    unsigned long* word_ptr = (unsigned long*)(offset_ptr + offset);
    unsigned char* byte_ptr = (unsigned char*)(word_ptr + word_size);
    unsigned char ch = (unsigned char)c;
    unsigned long byte = (unsigned long)ch;
    unsigned long word = (byte << 56) | (byte << 48) | (byte << 40) |
                         (byte << 32) | (byte << 24) | (byte << 16) |
                         (byte << 8) | byte;

    for (uint32_t i = 0; i < offset; i++)
        offset_ptr[i] = ch;

    for (uint32_t i = 0; i < word_size; i++)
        word_ptr[i] = word;

    for (uint32_t i = 0; i < byte_size; i++)
        byte_ptr[i] = ch;
}

/*
 * with `-nostdlib` compiler flag, we have to implement this function since
 * compiler may generate references to this function
 */
void *memcpy(void *dst, const void *src, uint32_t n)
{
    char* dst_cp = dst;
    char* src_cp = (char*)src;
    for (uint32_t i = 0; i < n; i++)
        dst_cp[i] = src_cp[i];

    return dst;
}