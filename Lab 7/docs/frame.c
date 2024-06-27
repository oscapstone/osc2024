#include "frame.h"
#include "type.h"
#include "list.h"
#include "uart.h"
#include "memory.h"


static void
bucket_init(byteptr_t bucket)
{
    list_head_ptr_t list = (list_head_ptr_t) bucket;
    INIT_LIST_HEAD(list);
}


static list_head_ptr_t
list_pop(const list_head_ptr_t list)
{
    if (list->next == list) return 0;
    list_head_ptr_t head = list->next;
    list_del_entry(head);
    return head;
}


static void
list_push(const list_head_ptr_t list, list_head_ptr_t entry)
{
    INIT_LIST_HEAD(entry);
    list_add_tail(entry, list);
}



#define FRAME_SIZE_ORDER    12
#define MIN_ALLOC_ORDER     FRAME_SIZE_ORDER

#define INDEX_LOG2(index)   ({31 - __builtin_clz(index);})

/*
 * This is the starting address of the address range for this allocator. Every
 * returned allocation will be an offset of this pointer from 0 to MAX_ALLOC.
 */
static byteptr_t            base_ptr;


/**
 * frame size = 4kb = 2^12b
 * The maximum allocation size is currently set to 1mb = 2^20b
 * thus the max order is 20-12=8 (256 pages)
 */
static uint32_t             max_alloc_order;
static uint32_t             max_alloc_size;
static uint32_t             total_frames;


/**
 * Frame Array Structure:   Binary Heap
 * example:
 *  - 16 frames = 2^4 frames
 *  - 5 freelists
 *  - 31 nodes, 15 internals
 *  - array size : 15 + 1
 * root index: 1
 * indices of leaves: 2^4 ~ 2^5-1
*/
// static uint8_t frame_array[(1 << (BUCKET_COUNT - 1))];
static byteptr_t            frame_array;

#define FRAME_SPLIT         (1 << 7)
#define FRAME_USED_L        (1 << 6)
#define FRAME_USED_R        (1 << 5)


/**
 * Freelists
*/
static uint32_t             bucket_count;
static list_head_ptr_t      buckets;



static uint32_t
parent_is_split(uint32_t index) 
{
    return frame_array[index >> 1] & FRAME_SPLIT;
}


static void
set_parent_split(uint32_t index) 
{
    frame_array[index >> 1] |= FRAME_SPLIT;
}


static void
clr_parent_split(uint32_t index) 
{
    frame_array[index >> 1] &= ~(FRAME_SPLIT);
}


static uint32_t
node_is_split(uint32_t index)
{
    return (INDEX_LOG2(index) > (bucket_count - 2)) ? 0 : frame_array[index] & FRAME_SPLIT;
}


static uint32_t
node_is_used(uint32_t index)
{
    return frame_array[index >> 1] & (FRAME_USED_L >> (index & 1));
}


static void
set_node_used(uint32_t index)
{
    frame_array[index >> 1] |= (FRAME_USED_L >> (index & 1));
}


static void
clr_node_used(uint32_t index)
{
    frame_array[index >> 1] &= ~(FRAME_USED_L >> (index & 1));
}


/**
 * 1. (index - (1 << index)) : the index of the corresponding level
 * 2. (the index of the level) << (MAX_ALLOC_ORDER - bucket) : the index of the bottom level
 */
static byteptr_t
index_to_ptr(uint32_t index, uint32_t bucket)
{
    return base_ptr + ((index - (1 << bucket)) << (max_alloc_order - bucket));
}


static uint32_t
ptr_to_index(byteptr_t ptr, uint32_t bucket)
{
    return ((ptr - base_ptr) >> (max_alloc_order - bucket)) + (1 << bucket);
}


static uint32_t 
order_for_request(uint32_t request) 
{
    uint32_t order = 0, size = 1 << MIN_ALLOC_ORDER;
    while (size < request) {
        ++order;
        size = size << 1;
    }
    return order;
}


static uint32_t
bucket_for_request(uint32_t request)
{
    return bucket_count - order_for_request(request) - 1;
}


static inline uint32_t
search_free_bucket(uint32_t bucket)
{
    while (bucket >= 0) {
        if (!list_empty(&buckets[bucket])) 
            return bucket;
        bucket--;
    }
    return -1;
}


static inline uint32_t
search_available_node(uint32_t index)
{
    while (index > 0) {
        if (node_is_used(index))
            return 0;
        if (parent_is_split(index))
            return index;
        index = index >> 1;
    }
    return index;
}


static void
split_up(uint32_t target, uint32_t index)
{
    while (target < index) {
        set_parent_split(index); 
        uint32_t bucket = INDEX_LOG2(index);
        byteptr_t buddy = index_to_ptr(index ^ 1, bucket);

        uart_printf("[DEBUG] split_up - bucket: %d, buddy: %d, addr: 0x%x\n", bucket, index ^ 1, buddy);

        list_push(&buckets[bucket], (list_head_ptr_t) buddy);
        index = (index >> 1);
    }
}


byteptr_t
add_init_free_block(byteptr_t ptr, uint32_t n_frames)
{
    uint32_t order = INT32_LEFTMOST_ORDER(n_frames);
    list_add((list_head_ptr_t) ptr, &buckets[(bucket_count - 1 - order)]);
    return (uint32_t) ptr + n_frames;
}


byteptr_t
init_frame_array(byteptr_t ptr, uint32_t n_frames)
{
    *ptr = *ptr | FRAME_SPLIT;
    return ptr + n_frames;
}


void
frame_system_init(byteptr_t base, byteptr_t limit)
{
    base_ptr = base;
    max_alloc_order = INDEX_LOG2((uint32_t) (limit - base));
    bucket_count = (max_alloc_order - FRAME_SIZE_ORDER + 1);
    buckets = smalloc(bucket_count * sizeof(list_head_t));
    uart_printf("[DEBUG] frame_system_init - max_alloc_order: %d, bucket count: %d\n", max_alloc_order, bucket_count);
    uart_printf("[DEBUG] frame_system_init - buckets addr: 0x%x, size: %d*%d bytes\n", buckets, bucket_count, sizeof(list_head_t));

    max_alloc_size = (uint32_t) memory_align(limit, FRAME_SIZE_ORDER) 
                    - (uint32_t) memory_align(base, FRAME_SIZE_ORDER);
    uart_printf("[DEBUG] frame_system_init - max_alloc_size: 0x%x\n", max_alloc_size);

    total_frames = max_alloc_size >> FRAME_SIZE_ORDER;
    frame_array = smalloc(total_frames * sizeof(byte_t));
    for (int i = 0; i < total_frames; ++i) frame_array[i] = 0;
    uart_printf("[DEBUG] frame_system_init - frame_array addr: 0x%x, size: %d*%d bytes\n", frame_array, (1 << (bucket_count - 1)), sizeof(byte_t));

    for (int i = 0; i < bucket_count; ++i)
        bucket_init((byteptr_t) &buckets[i]);

    memseg_process(add_init_free_block, buckets, total_frames);
    memseg_process(init_frame_array, frame_array, total_frames);

    // byteptr_t ptr = base_ptr;
    // uint32_t frames = total_frames;     // 3C000

    // while (frames != 0) {
    //     /**
    //      * if total pages is 0x3C000
    //      *    0x20000(17), 0x10000(16), 0x8000(15), 0x4000(14)
    //     */
        
    //     uint32_t order = INT32_LEFTMOST_ORDER(frames);
    //     list_add((list_head_ptr_t) ptr, &buckets[(bucket_count - 1 - order)]);
    //     ptr = (uint32_t) ptr + (1 << order);
    //     frames = frames & ~(1 << order);
    // }

    // frame_array[0] |= FRAME_SPLIT;
}


byteptr_t 
frame_alloc(uint32_t request)
{
    /**
     * calculate the ideal order level
     */
    uint32_t target_bucket = bucket_for_request(request);
    
    /**
     * find the available order level if there's a free block smallest larger than request
     */
    uint32_t bucket = search_free_bucket(target_bucket);
    if (bucket < 0) return 0;
    
    /**
     * pop the free block
     */
    byteptr_t ptr = (byteptr_t) list_pop(&buckets[bucket]);
    uint32_t target_index = ptr_to_index(ptr, target_bucket);

    /**
     * get the free index of the block
     */
    uint32_t index = ptr_to_index(ptr, bucket);
    
    /**
     * iteratively split the block into the ideal order level, and update the index
     */
    set_node_used(target_index);
    split_up(index, target_index);

    return ptr;
}


byteptr_t
frame_request(uint32_t count)
{
    uint32_t request = count << FRAME_SIZE_ORDER;
    if (request > max_alloc_size) { return 0; }
    return frame_alloc(request);
}
    

byteptr_t
frame_alloc_addr(uint32_t addr, uint32_t count)
{
    byteptr_t ptr = memory_align(addr, FRAME_SIZE_ORDER);

    uart_printf("[DEBUG] frame_alloc_addr - addr: %x\n", addr);

    /**
     * calculate the ideal order level and index of the block
     */
    uint32_t target_bucket = bucket_for_request(count << FRAME_SIZE_ORDER);
    uint32_t target_index = ptr_to_index(ptr, target_bucket);

    uart_printf("[DEBUG] frame_alloc_addr - target index: %d, bucket: %d\n", target_index, target_bucket);

    /**
     * find available free block 
     */
    uint32_t index = search_available_node(target_index);
    if (index == 0 || node_is_used(index)) { return 0; }
    uint32_t bucket = INDEX_LOG2(index);

    /**
     * pop the free block
     */
    ptr = index_to_ptr(index, bucket);
    list_del_entry((list_head_ptr_t) ptr);

    /**
     * split buddies
     */
    set_node_used(target_index);

    uart_printf("[DEBUG] frame_alloc_addr - index: %d, target: %d, addr: 0x%x\n", index, target_index, addr);
    split_up(index, target_index);

    return ptr;
}


#define identify_ptr(ptr, index, bucket) \
    bucket = bucket_count - 1; index = ptr_to_index(ptr, bucket);\
    while (bucket > 0 && !parent_is_split(index)) {index = index >> 1; --bucket;}


void 
frame_release(byteptr_t ptr)
{
    if (!ptr) return;
    
    /**
     * identify the index and order level
    */
    uint32_t index, bucket;
    identify_ptr(ptr, index, bucket);
    clr_node_used(index);

    /**
     * merge the buddy by bottom-up if it's unused
    */
    uint32_t buddy = index ^ 1;
    while (bucket > 0 && !node_is_used(buddy) && !node_is_split(buddy)) {
        byteptr_t buddy_ptr = index_to_ptr(buddy, bucket);
        list_del_entry((list_head_ptr_t) buddy_ptr);
        clr_parent_split(index);
        index = index >> 1; --bucket; buddy = index ^ 1;
    }
    
    list_add((list_head_ptr_t) ptr, &buckets[bucket]);
}


uint32_t next_child(byteptr_t ptr, uint32_t index)
{
    while(!parent_is_split(index)) {
        index = index >> 1;
    }
}

static byteptr_t
print_buddy_info(byteptr_t ptr, uint32_t n_frames)
{
    byteptr_t mem_base = (uint32_t) base_ptr + (((uint32_t)ptr - (uint32_t)frame_array) << FRAME_SIZE_ORDER);

    uint32_t i = n_frames;
    while (i > 0) {
        
    }

}


void
frame_print_info()
{
    #define block_type(index) ({(node_is_used(index)) ? "allocated" : "free"; })
 
    uart_printf("\n==== FRAME SYSTEM INFO ====\n");
    uart_printf("base:     0x%8x\n", base_ptr);
    uart_printf("limit:    0x%8x\n", base_ptr + max_alloc_size);
    uart_printf("freelist: %d buckets\n", bucket_count);

    uart_printf("\n==== MEMORY FRAME LAYOUT ====\n");
    
    // byteptr_t ptr = base_ptr;
    // uint32_t index, bucket;
    // while (ptr < base_ptr + max_alloc_size) {
    //     identify_ptr(ptr, index, bucket);
    //     uint32_t order = (bucket_count-1)-bucket;
    //     uart_printf("0x%8x  |%6dkb | %s\n", ptr, (1 << order + 2), block_type(index));
    //     ptr = index_to_ptr(index+1, bucket);
    // }

    // uint32_t count = total_frames;
    // byteptr_t ptr = frame_array;
    // while (count != 0) {
    //     uint32_t order = INT32_LEFTMOST_ORDER(count);
        


    //     ptr = (uint32_t) ptr + (1 << order);
    //     count = count & ~(1 << order);
    // }


    uart_printf("\n==== FREE FRAME LISTS ====\n");
    for (int i = 0; i < bucket_count; ++i) {
        if (!list_empty(&buckets[i])) {
            uart_printf("bucket: %d, size: %dkb\n", i, 1 << (bucket_count-1 - i + 2));
            list_head_ptr_t pos;
            list_for_each(pos, (list_head_ptr_t) &buckets[i])
                uart_printf(" +- 0x%8x\n", pos);
        }
    }
    uart_printf("\n====================\n\n");
}
