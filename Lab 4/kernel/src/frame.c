#include "frame.h"
#include "type.h"
#include "list.h"
#include "uart.h"
#include "memory.h"


#define FRAME_SIZE_ORDER    12


static byteptr_t            base_ptr;
static uint32_t             max_alloc_size;
static uint32_t             max_alloc_order;

static uint32_t             total_frames;
static byteptr_t            frame_array;


#define FRAME_ALLOCATED     0x40
#define FRAME_PRESERVED     0x20
#define FRAME_BUDDY         0x60


#define FRAME_IS_ALLOCATED(x)       ({frame_array[x] == FRAME_ALLOCATED;})
#define FRAME_IS_PRESERVED(x)       ({frame_array[x] == FRAME_PRESERVED;})
#define FRAME_IS_BUDDY(x)           ({frame_array[x] == FRAME_BUDDY;})

#define FRAME_IS_FREE(x)            ({(frame_array[x] & 0xE0) == 0x00;})
#define FRAME_ORDER(x)              ({frame_array[x];})

#define FRAME_SET_ORDER(x, order)   frame_array[x] = 0x1F & order
#define FRAME_OWNER(index, order)   ({index & ~((1 << order) - 1);})
#define BUDDY_INDEX(index, order)   ({index ^ (1 << order);})


/**
 * Freelists
*/
#define BUCKET_COUNT        max_alloc_order + 1

static list_head_ptr_t      buckets;


static byteptr_t
index_to_ptr(uint32_t index)
{
    return (byteptr_t) (((uint64_t) base_ptr) + (index << FRAME_SIZE_ORDER));
}


static uint32_t
ptr_to_index(byteptr_t ptr)
{
    return ((uint32_t) ptr) >> FRAME_SIZE_ORDER;
}


static void
bucket_init(byteptr_t bucket)
{
    list_head_ptr_t list = (list_head_ptr_t) bucket;
    INIT_LIST_HEAD(list);
}


/**
 * pop a free block from the designate bucket
*/
static byteptr_t
bucket_pop(const byteptr_t bucket)
{
    list_head_ptr_t head = (list_head_ptr_t) bucket;
    if (head->next == head) return 0;
    list_head_ptr_t first = head->next;
    list_del_entry(first);
    return (byteptr_t) first;
}


/**
 * push a free block by a designate index
 * should set the order value properly before call this function
*/
static void
buckets_push(uint32_t index)
{
    byteptr_t block = index_to_ptr(index);
    uint32_t order = FRAME_ORDER(index);

    INIT_LIST_HEAD((list_head_ptr_t) block);
    list_add_tail((list_head_ptr_t) block, (list_head_ptr_t) &buckets[order]);  
}


static void
buckets_remove(uint32_t index)
{
    list_del_entry((list_head_ptr_t) index_to_ptr(index));
}


static uint32_t
frame_split(uint32_t index, uint32_t order) 
{
    if (!FRAME_IS_FREE(index)) { return 0; }    
    if (order == 0) { return 0; }
    uint32_t buddy = BUDDY_INDEX(index, (order - 1));
    if (index > (total_frames - 1) || buddy > (total_frames - 1)) return 0;
    FRAME_SET_ORDER(buddy, (order - 1));
    FRAME_SET_ORDER(index, (order - 1));
    buckets_push(buddy);
    return 1;
}


static uint32_t
frame_split_down(uint32_t index)
{
    if (!FRAME_IS_FREE(index)) return 0;
    uint32_t order = FRAME_ORDER(index);
    while (frame_split(index, order)) { 
        order--; 
    }
    return 1;
}


static uint32_t
frame_mergeable(uint32_t index, uint32_t buddy)
{
    if (index > (total_frames - 1) || buddy > (total_frames - 1)) return 0;
    if (FRAME_IS_FREE(index) &&  FRAME_ORDER(index) == FRAME_ORDER(buddy)) return 1;
    return 0;
}


static uint32_t
frame_merge(uint32_t index, uint32_t order)
{
    if (!FRAME_IS_FREE(index)) return 0;
    uint32_t buddy = BUDDY_INDEX(index, order);

    if (!frame_mergeable(index, buddy)) return 0;
    frame_array[buddy] = FRAME_BUDDY;
    frame_array[index] = order + 1;
    return 1;
}


byteptr_t 
frame_alloc()
{
    int32_t order = 0;
    while (order < BUCKET_COUNT && list_empty(&buckets[order])) { order++; }
    if (order < 0) return 0;

    byteptr_t block = bucket_pop((byteptr_t) &buckets[order]);
    uint32_t index = ptr_to_index(block);
    frame_split_down(index);
    frame_array[index] = FRAME_ALLOCATED;
    return block;
}


void 
frame_release(byteptr_t ptr)
{
    uint32_t index = ptr_to_index(ptr);
    if (!FRAME_IS_ALLOCATED(index)) return;

    frame_array[index] = 0;

    uint32_t order = 0;
    uint32_t buddy = BUDDY_INDEX(index, order);
    
    while (frame_mergeable(index, buddy)) {

        uart_printf("[DEBUG] order: %d, index: %x, buddy: %x\n", order, index, buddy);
        buckets_remove(buddy);
        index = FRAME_OWNER(index, order);
        buddy = BUDDY_INDEX(index, order);
        frame_array[buddy] = FRAME_BUDDY;
        frame_array[index] = order + 1;
        buddy = BUDDY_INDEX(index, ++order);
    }

    buckets_push(index);
}


#define FRAME_TYPE(index) ({(FRAME_IS_PRESERVED(index)) ? "preserved" : (FRAME_IS_ALLOCATED(index)) ? "allocated" : "free";})


static void
print_memory_layout()
{
    uart_printf("\n==== MEMORY FRAME LAYOUT ====\n");
    uint32_t index = 0;
    while (index < total_frames) {
        uint32_t end = index + 1;
        if (FRAME_IS_FREE(index)) {
            while (FRAME_IS_FREE(end) || FRAME_IS_BUDDY(end)) end++;
        } 
        else {
            while (frame_array[index] == frame_array[end]) end++;
        }
        uart_printf("0x%8x | %dkb\t| %s\n", index_to_ptr(index), (end - index) << 2, FRAME_TYPE(index));
        index = end;
    }
}


static void
print_buddy_layout()
{
    uart_printf("\n==== MEMORY BUDDY LAYOUT ====\n");
    uint32_t index = 0;
    while (index < total_frames) {
        if (FRAME_IS_FREE(index)) {
            uint32_t order = frame_array[index];
            uart_printf("0x%8x | %dkb\t| free\n", index_to_ptr(index), (1 << (order + 2)));
            index += (1 << order);
        } 
        else {
            uint32_t end = index + 1;
            while (frame_array[index] == frame_array[end]) end++;
            uart_printf("0x%8x | %dkb\t| %s\n", index_to_ptr(index), (end - index) << 2, FRAME_TYPE(index));
            index = end;
        }
    }
}


static void
print_free_lists()
{
    uart_printf("\n==== FREE FRAME LISTS ====\n");
    for (int i = 0; i < BUCKET_COUNT; ++i) {
        if (!list_empty(&buckets[i])) {
            uart_printf("bucket: %d, size: %dkb\n", i, 1 << (i + 2));
            list_head_ptr_t pos = ((list_head_ptr_t) &buckets[i])->next;
            list_for_each(pos, (list_head_ptr_t) &buckets[i])
                uart_printf(" +- 0x%8x\n", pos);
        }
    }
}


void
frame_print_info()
{ 
    uart_printf("\n==== FRAME SYSTEM INFO ====\n");
    uart_printf("start:       0x%8x\n", base_ptr);
    uart_printf("limit:       0x%8x\n", base_ptr + max_alloc_size);
    uart_printf("frame_array: 0x%8x | 0x%x bytes\n", frame_array, total_frames);
    uart_printf("free_list:   0x%8x | %d buckets\n", buckets, BUCKET_COUNT);
    
    print_memory_layout();
    print_buddy_layout();
    print_free_lists();
    uart_printf("\n");
}


void
frame_system_init_frame_array(uint32_t start_addr, uint32_t end_addr)
{
    start_addr = (uint32_t) memory_align((byteptr_t) (start_addr | 0l), FRAME_SIZE_ORDER);
    end_addr = (uint32_t) memory_align((byteptr_t) (end_addr | 0l), FRAME_SIZE_ORDER);
    
    max_alloc_size = (end_addr - start_addr);
    total_frames = max_alloc_size >> FRAME_SIZE_ORDER;
    base_ptr = (byteptr_t) (start_addr | 0l);

    frame_array = smalloc(total_frames * sizeof(byte_t));
    for (int i = 0; i < total_frames; ++i) { if (FRAME_IS_FREE(i)) frame_array[i] = 0; }
}


uint32_t
frame_system_preserve(uint32_t start, uint32_t end)
{
    uint32_t index_s = ptr_to_index(memory_align((byteptr_t) (start | 0l), FRAME_SIZE_ORDER));
    uint32_t index_e = ptr_to_index(memory_padding((byteptr_t) (end | 0l), FRAME_SIZE_ORDER));

    if (index_s >= total_frames || index_e > total_frames) 
        return 0;
    while (index_s < index_e) {
        frame_array[index_s++] = FRAME_PRESERVED;
    }
    return 1;
}


void
frame_system_init_free_blocks()
{    
    /**
     * create the freelists
    */
    max_alloc_order = INT32_LEFTMOST_ORDER(total_frames);
    buckets = (list_head_ptr_t) smalloc(BUCKET_COUNT * sizeof(list_head_t));
    for (int i = 0; i < BUCKET_COUNT; ++i) 
        bucket_init((byteptr_t) &buckets[i]);


    uart_printf("frame_system_init_free_blocks - %d, %d\n", frame_array[0], frame_array[1]);
    
    /**
     * merge all free blocks as high order as possible
    */
    for (uint32_t order = 0; order < max_alloc_order; order++)
        for (uint32_t index = 0; index < total_frames; index += (1 << (order + 1))) 
            frame_merge(index, order);

uart_printf("frame_system_init_free_blocks - %d, %d\n", frame_array[0], frame_array[1]);

    /**
     * push all the free blocks into freelists
    */
    for (int i = 0; i < total_frames; ++i)
        if (FRAME_IS_FREE(i))
            buckets_push(i);
}
