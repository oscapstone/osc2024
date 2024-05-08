#include "memory.h"
#include "bool.h"
#include "cpio.h"
#include "dtb.h"
#include "list.h"
#include "mini_uart.h"

extern char kernel_start;
extern char kernel_end;

extern char heap_begin;
extern char heap_end;

static char* heap_ptr = &heap_begin;

/* Get from device tree */
static uintptr_t usable_mem_start;
static uintptr_t usable_mem_length;
static uintptr_t usable_mem_end;

static uintptr_t cpio_start;
static uintptr_t cpio_end;

static uintptr_t dtb_start;
static uintptr_t dtb_end;

/* reserved memory region - spin table */
#define DTS_MEM_RESERVED_START  0x0
#define DTS_MEM_RESERVED_LENGTH 0x1000
#define DTS_MEM_RESERVED_END    (DTS_MEM_RESERVED_START + DTS_MEM_RESERVED_LENGTH)

int mem_init(uintptr_t dtb_ptr)
{
    /* dtb addreses */
    if (fdt_init(dtb_ptr))
        goto fail;

    dtb_start = get_dtb_start();
    dtb_end = get_dtb_end();

#if defined(DEBUG)
    uart_printf("dtb_start: 0x%x\n", dtb_start);
    uart_printf("dtb_end: 0x%x\n", dtb_end);
#endif

    /* cpio addresses */
    if (cpio_init())
        goto fail;

    cpio_start = get_cpio_start();
    cpio_end = get_cpio_end();

#if defined(DEBUG)
    uart_printf("cpio_start: 0x%x\n", cpio_start);
    uart_printf("cpio_end: 0x%x\n", cpio_end);
#endif

    /* usable memory region */
    // find the #address-cells and #size-cells (for reg property of the children
    // node), and use this information to extract the reg property of memory
    // node.
    if (fdt_traverse(fdt_find_root_node) || fdt_traverse(fdt_find_memory_node))
        goto fail;

    usable_mem_start = get_usable_mem_start();
    usable_mem_length = get_usable_mem_length();
    usable_mem_end = usable_mem_start + usable_mem_length;

#if defined(DEBUG)
    uart_printf("usable memory start: 0x%x\n", usable_mem_start);
    uart_printf("usable memory end: 0x%x\n", usable_mem_end);
#endif

    return 1;

fail:
    return 0;
}

void* mem_alloc(uint64_t size)
{
    if (!size)
        return NULL;

    size = (uint64_t)mem_align((char*)size, 8);

    if (heap_ptr + size > &heap_end)
        return NULL;

    char* ptr = heap_ptr;
    heap_ptr += size;

    return ptr;
}

void mem_free(void* ptr)
{
    // TODO
    return;
}

/*
 * Page allocator - Buddy System
 * Page size = 4KB (defined in `mm.h`)
 */
/*
 * Every allocation needs an 1-byte header to store the order of the allocation
 * size. The address returned by `alloc_pages` is the address right after the
 * header.
 */
#define HEADER_SIZE 1

// HACK: For a non-power-of-2 size memory region, we simply treat it as a
// nearest higher power-of-2 memory region, and check if the address is beyond
// the real upper bound while do some operation

static size_t MAX_ALLOC, MAX_ALLOC_LOG2;
static size_t MIN_ALLOC, MIN_ALLOC_LOG2;

#define BUCKET_COUNT \
    (MAX_ALLOC_LOG2 - MIN_ALLOC_LOG2 + 1)  // How many order in total

/*
 * free lists for each order
 * index is calculated by `MAX_ALLOC_LOG2 - order`
 */
static struct list_head** free_list;

#define get_order_from_bucket(bucket) (MAX_ALLOC_LOG2 - (bucket))
#define get_bucket_from_order(order)  (MAX_ALLOC_LOG2 - (order))

/*
 * This array is a bitmap which represents linearized binary tree nodes'
 * status (split or not) Every possible allocation order larger than
 * MIN_ALLOC_LOG2 has a node in this tree (because we only care about the
 * parent nodes)
 *
 * Each node in this tree can be in one of the following states:
 * - UNUSED (both children are UNUSED)
 * - SPLIT (one child is UNUSED and the other child isn't)
 * - USED (neither children are UNUSED)
 *
 * It turns out we have enough information to distinguish between UNUSED and
 * USED from context, so we only need to store SPLIT or not.
 *
 * SPLIT is basically the XOR of the two children nodes' UNUSED flags.
 * 0 - NOT SPLIT (Both children nodes are UNUSED or USED)
 * 1 - SPLIT (One of the children node is UNUSED)
 *
 */
/*
 * Total nodes count (without minimum order)
 * = 2^0 + 2^1 + 2^2 + ... + 2^(MAX_ALLOC_LOG2 - MIN_ALLOC_LOG2 - 1)
 * = 2^(MAX_ALLOC_LOG2 - MIN_ALLOC_LOG2 - 1 + 1) - 1
 * = 2^(BUCKET_COUNT - 1) - 1
 */
#define NODE_IS_SPLIT_SIZE ((1 << (BUCKET_COUNT - 1)) >> 3)
static uint8_t* node_is_split;


/* This array is a bitmap which represents the lowest level of the binary tree
 * nodes, whose order is MIN_ALLOC_LOG2, it is meant to be used in the reserve
 * memory functions
 */
/*
 * Total Lowest level nodes count
 * = 2^(MAX_ALLOC_LOG2 - MIN_ALLOC_LOG2)
 * = 2^(BUCKET_COUNT - 1)
 */
#define NODE_IS_RESERVED_SIZE ((1 << (BUCKET_COUNT - 1)) >> 3)
static uint8_t* node_is_reserved;

// if anything is reserved? this is used for `start_reserve_pages`.
static bool reserve_flag;

#define get_parent(index)      (((index)-1) >> 1)
#define get_left_child(index)  (((index) << 1) + 1)
#define get_right_child(index) (((index) << 1) + 2)
#define get_sibling(index)     ((((index)-1) ^ 1) + 1)

#define is_split(index)             (node_is_split[(index) >> 3] & (1 << ((index)&7)))
#define flip_is_split(index)        (node_is_split[(index) >> 3] ^= (1 << ((index)&7)))
#define parent_is_split(index)      (is_split(get_parent((index))))
#define flip_parent_is_split(index) (flip_is_split(get_parent((index))))

#define is_reserved(index)                                            \
    (node_is_reserved[(((index) - (1 << MAX_ALLOC_LOG2) + 1) >> 3)] & \
     (1 << (((index) - (1 << MAX_ALLOC_LOG2) + 1) & 7)))

#define flip_is_reserved(index)                                        \
    (node_is_reserved[(((index) - (1 << MAX_ALLOC_LOG2) + 1) >> 3)] ^= \
     (1 << (((index) - (1 << MAX_ALLOC_LOG2) + 1) & 7)))

static uint8_t* base_ptr;
static uint8_t* max_ptr;

// The bucket is required to avoid the need to derive it
// from the index using a loop.
#define get_ptr_from_index(index, bucket)        \
    (base_ptr + (((index) - (1 << (bucket)) + 1) \
                 << (get_order_from_bucket((bucket)) + PAGE_SHIFT)))

// The bucket is required  since there may be many nodes
// that all map to the same address.
#define get_index_from_ptr(ptr, bucket)                 \
    ((((uintptr_t)(ptr) - (uintptr_t)base_ptr) >>       \
      (get_order_from_bucket((bucket)) + PAGE_SHIFT)) + \
     (1 << (bucket)) - 1)

/* align a value to the a higher power-of-two*/
static inline size_t align_up_pow2(size_t n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

#define PAGE_ALIGN_DOWN(size) ((size) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(size)   mem_align(((void*)(size)), PAGE_SIZE)
#define LOG2LL(value) \
    (((sizeof(long long) << 3) - 1) - __builtin_clzll((value)))

/*
 * Given the requested size, return the index of the
 * smallest bucket that can satisfy the request. (Assume
 * request <= MAX_ALLOC)
 */
static size_t bucket_for_request(size_t request)
{
    size_t size = (size_t)PAGE_ALIGN_UP(request);  // page-alignment
    size_t pages =
        align_up_pow2(size >> PAGE_SHIFT);        // align number of page to 2^n
    return get_bucket_from_order(LOG2LL(pages));  // index of the bucket
}

/* remove the last node from the list */
static struct list_head* list_pop(struct list_head* head)
{
    if (list_empty(head))
        return NULL;

    struct list_head* last = head->prev;
    list_del_init(last);

    return last;
}

/* Add a node to the last of the list */
static void list_push(struct list_head* head, struct list_head* node)
{
    list_add_tail(node, head);
}

/*
 * update max_ptr, this make sure we do not get out of bound (exceed
 * usable_mem_end)
 */
static int update_max_ptr(uint8_t* new_val)
{
    if (new_val > max_ptr) {
        if ((uintptr_t)new_val >= usable_mem_end)
            return 0;
        max_ptr = new_val;
    }
    return 1;
}

/* allocate pages from our page allocator */
uint8_t* alloc_pages(size_t request)
{
#if defined(DEBUG)
    uart_printf("\n===================================\n");
    uart_printf("alloc pages: size 0x%x\n", request);
    uart_printf("===================================\n");
#endif

    if (request + HEADER_SIZE > (MAX_ALLOC << PAGE_SHIFT)) {
#if defined(DEBUG)
        uart_printf("Request size too LARGE!! fail.\n");
#endif
        return NULL;
    }

    // If we alloc_pages(0), we should return a pointer
    // that can be passed to free_pages() later
    request += !request;  // +1 if request == 0,
                          // +0, otherwise.

    // Find the smallest bucket that can satisfy the (request + HEADER)
    size_t bucket = bucket_for_request(request + HEADER_SIZE);
    size_t original_bucket = bucket;

#if defined(DEBUG)
    uart_printf("Request order: %d\n", get_order_from_bucket(bucket));
#endif

    /*
     * Search for a bucket with a non-empty free list
     * that's as large or larger than what we need. If the
     * bucket we get is larger, split it to get a match.
     */
    // since the type of bucket is of size_t (aka unsigned long long), we have
    // to make sure it doesn't overflow.
    while (bucket + 1 != 0) {
        // Try to find a free block in the current bucket free list
        uint8_t* ptr = (uint8_t*)list_pop(free_list[bucket]);

        // If the free list for this bucket is empty, check
        // the free list for the next larger bucket instead
        if (!ptr) {
#if defined(DEBUG)
            uart_printf(
                "Free list for order %d is empty. "
                "Moving to next order\n",
                get_order_from_bucket(bucket));
#endif
            bucket--;
            continue;
        }

#if defined(DEBUG)
        uart_printf("Found free block of order  %d !!\n",
                    get_order_from_bucket(bucket));
#endif

        /*
         * Try to expand the space first before going any
         * further. If we have run out of space, put this
         * block back on the free list and fail.
         */
        size_t size = (size_t)1 << get_order_from_bucket(bucket);
        size_t bytes_needed = bucket < original_bucket
                                  ? (size >> 1) + sizeof(struct list_head)
                                  : size;

        if (!update_max_ptr(ptr + bytes_needed)) {
#if defined(DEBUG)
            uart_printf("Out of memory. Allocation fail\n");
#endif
            list_push(free_list[bucket], (struct list_head*)ptr);
            return NULL;
        }

        /*
         * If we got a node from the free list, change the
         * node from UNUSED to USED. We do this by flipping
         * the split flag of the parent node. (because the
         * split flag is the XOR of the UNUSED flags of both
         * children), and our UNUSED flag has just changed.
         */
        size_t i = get_index_from_ptr(ptr, bucket);
        if (i)
            flip_parent_is_split(i);

        /*
         * If the node we get is larger than what we need,
         * split it down until we get the correct size and
         * put the new unused child nodes on the free list in
         * the corresponding bucket. This is done by
         * repeatedly moving to the left child node,
         * splitting the parent, and then adding the right
         * child to the free list.
         */
        while (bucket < original_bucket) {
#if defined(DEBUG)
            uart_printf(
                "Splitting order %d page into half: 0x%x - "
                "0x%x",
                get_order_from_bucket(bucket),
                (uintptr_t)get_ptr_from_index(i, bucket),
                (uintptr_t)get_ptr_from_index(i + 1, bucket));
#endif
            i = get_left_child(i);
            bucket++;

            flip_parent_is_split(i);
            list_push(free_list[bucket],
                      (struct list_head*)get_ptr_from_index(i + 1, bucket));

#if defined(DEBUG)
            uart_printf(" => 0x%x - 0x%x and 0x%x - 0x%x\n",
                        (uintptr_t)get_ptr_from_index(i, bucket),
                        (uintptr_t)get_ptr_from_index(i + 1, bucket),
                        (uintptr_t)get_ptr_from_index(i + 1, bucket),
                        (uintptr_t)get_ptr_from_index(i + 2, bucket));
#endif
        }

        /* put order into the allocated page address */
        *(uint8_t*)ptr = get_order_from_bucket(bucket);
#if defined(DEBUG)
        uart_printf("Allocated %d pages at 0x%x\n", 1 << *(uint8_t*)ptr, ptr);
#endif

        /* return the address after the header */
        return ptr + HEADER_SIZE;
    }

    return NULL;
}

void free_pages(uint8_t* ptr)
{
#if defined(DEBUG)
    uart_printf("\n===================================\n");
    uart_printf("free pages: address 0x%x\n", (uintptr_t)ptr - HEADER_SIZE);
    uart_printf("===================================\n");
#endif

    /* if the ptr is NULL of it exceed usable_mem_end, stop*/
    if (!ptr || (uintptr_t)ptr >= usable_mem_end)
        return;

    /*
     * The given address is returned by `alloc_pages`, so
     * we have to get back to the actual address of the
     * node by subtracting the header size.
     */
    ptr -= HEADER_SIZE;

#if defined(DEBUG)
    uart_printf("free address 0x%x with %d pages\n", (uintptr_t)ptr,
                1 << (*(uint8_t*)ptr));
    uart_printf("Start traversing for coalesce\n");
#endif
    size_t bucket = get_bucket_from_order(*(uint8_t*)ptr);
    size_t i = get_index_from_ptr(ptr, bucket);

    /*
     * Traverse up to the root node, flipping USED blocks
     * to UNUSED and mergin UNUSED buddies together into a
     * single UNUSED parent.
     */
    while (i != 0) {
#if defined(DEBUG)
        uart_printf("Search in order %d free list\n",
                    get_order_from_bucket(bucket));
#endif
        // flip the parent's split flag since the current
        // node UNUSED flag has changed.
        flip_parent_is_split(i);

        // If the parent is SPLIT, that means our buddy is
        // not UNUSED, we can't merge the buddy, so we're
        // done.
        if (parent_is_split(i)) {
#if defined(DEBUG)
            uart_printf("Buddy is in used. Stop traversing\n");
#endif
            break;
        }

        // Since the buddy is UNUSED. Remove the buddy from
        // free list in corresponding bucket. and continue
        // traversing up to the root node.

#if defined(DEBUG)
        uart_printf("Found buddy at 0x%x. Continue to traverse\n",
                    get_ptr_from_index(get_sibling(i), bucket));
#endif
        uintptr_t buddy_addr =
            (uintptr_t)get_ptr_from_index(get_sibling(i), bucket);

        /* check if the buddy's address exceeds the usable_mem_end */
        if (buddy_addr < usable_mem_end)
            list_del_init((struct list_head*)buddy_addr);

        i = get_parent(i);
        bucket--;
    }

    // Add the merged block to the end of free list in the
    // corresponding bucket.
#if defined(DEBUG)
    uart_printf("Merged block at 0x%x with order %d\n",
                get_ptr_from_index(i, bucket), get_order_from_bucket(bucket));
#endif

    uintptr_t merged_addr = (uintptr_t)get_ptr_from_index(i, bucket);

    /* check if the merged address exceeds the usable_mem_end */
    if (merged_addr < usable_mem_end)
        list_push(free_list[bucket], (struct list_head*)merged_addr);
}

/*
 * register the reserve region, reserve page_align(start) <= address <=
 * page_align(end), it needs to be called before `start_init_pages` function.
 */
static void register_reserve_pages(uintptr_t start, uintptr_t end)
{
#if defined(DEBUG)
    uart_printf("register reserve, start: 0x%x, end: 0x%x\n", start, end);
#endif

    if (end < start) {
#if defined(DEBUG)
        uart_printf(
            "INVALID ADDRESS: start address is larger than end address!!\n");
#endif
        return;
    }

#if defined(DEBUG)
    uart_printf("calculate page alignment address\n");
#endif

    start = (uintptr_t)PAGE_ALIGN_DOWN(start - (uintptr_t)base_ptr) +
            (uintptr_t)base_ptr;
    end = (uintptr_t)PAGE_ALIGN_UP(end - (uintptr_t)base_ptr) +
          (uintptr_t)base_ptr;

#if defined(DEBUG)
    uart_printf("page-alignment address, start_addr: 0x%x, end_addr: 0x%x\n",
                start, end);
    uart_printf("calculate page index\n");
#endif

    size_t start_idx = get_index_from_ptr(start, MAX_ALLOC_LOG2);
    size_t end_idx = get_index_from_ptr(end - PAGE_SIZE, MAX_ALLOC_LOG2);

#if defined(DEBUG)
    uart_printf("start reserve, start_idx: 0x%x, end_idx: 0x%x\n", start_idx,
                end_idx);
#endif

    size_t i = start_idx;

    while (i <= end_idx) {
#if defined(DEBUG)
        uart_printf("check reserved idx: 0x%x\n", i);
#endif
        if (!is_reserved(i)) {
#if defined(DEBUG)
            uart_printf("reserved\n");
#endif
            flip_is_reserved(i);
#if defined(DEBUG)
            uart_printf("start to split the ancestors\n");
#endif
            size_t temp = i;
            while (temp) {
                flip_parent_is_split(temp);
                if (!parent_is_split(temp))
                    break;
                temp = get_parent(temp);
            }
        }
        i++;
    }
#if defined(DEBUG)
    uart_printf("register done\n");
    uart_printf("\n");
#endif
    reserve_flag = true;
}

/*
 * start reserving pages and push all the free blocks onto the free list with
 * its corresponding bucket, if there is any call of `register_reserve_pages`
 * function, this function must be called after all of them.
 */
static void start_init_pages(void)
{
#if defined(DEBUG)
    uart_printf("start_reserve_pages\n");
#endif

    /*
     * if there is no page need to be reserved, push the whole memory region
     * onto the free list with maximum order
     */
    if (!reserve_flag) {
        list_push(free_list[0], (struct list_head*)base_ptr);
        return;
    }

    // leftmost node in the lowest level of the binary tree
    size_t i = (1 << MAX_ALLOC_LOG2) - 1;

    // rightmost node in the lowest level of the binary tree
    size_t last = (1 << (MAX_ALLOC_LOG2 + 1)) - 2;

    while (i <= last) {
#if defined(DEBUG)
        uart_printf("check reserved idx: 0x%x\n", i);
#endif
        if (is_reserved(i)) {
#if defined(DEBUG)
            uart_printf("page already reserved!\n");
#endif
            i++;
            continue;
        }

#if defined(DEBUG)
        uart_printf("find first split\n");
#endif

        size_t first_split = i;
        size_t curr_bucket = MAX_ALLOC_LOG2;
        while (first_split && !parent_is_split(first_split)) {
            first_split = get_parent(first_split);
            curr_bucket--;
        }

        if (first_split) {
#if defined(DEBUG)
            uart_printf(
                "found first split, index: 0x%x, order: %d, addr: 0x%x\n",
                first_split, get_order_from_bucket(curr_bucket),
                (uintptr_t)get_ptr_from_index(first_split, curr_bucket));
            uart_printf("push onto free list\n");
#endif
            list_push(free_list[curr_bucket],
                      (struct list_head*)get_ptr_from_index(first_split,
                                                            curr_bucket));
#if defined(DEBUG)
            uart_printf("update index\n");
#endif
            i += (1 << get_order_from_bucket(curr_bucket));
        } else {
            i++;
        }
#if defined(DEBUG)
        uart_printf("\n");
#endif
    }
#if defined(DEBUG)
    uart_printf("\n");
#endif
}

/* zero out a given array with given size*/
static void init_array(uint8_t* array, size_t total_bytes)
{
    size_t stride = sizeof(unsigned long long);
    size_t count = (total_bytes & ~(stride - 1)) >> LOG2LL(stride);
    size_t remain = total_bytes & (stride - 1);

    unsigned long long* ptr = (unsigned long long*)array;
    while (count--)
        *ptr++ = 0;

    while (remain--)
        *(uint8_t*)ptr = 0;
}

#define INIT_NODE_IS_SPLIT() init_array(node_is_split, NODE_IS_SPLIT_SIZE)
#define INIT_NODE_IS_RESERVED() \
    init_array(node_is_reserved, NODE_IS_RESERVED_SIZE)

/*
 * Initialize the buddy system allocator. must be called
 * before any allocation happen.
 */
void buddy_init(void)
{
    base_ptr = max_ptr = (uint8_t*)usable_mem_start;

    MIN_ALLOC = 1;
    MIN_ALLOC_LOG2 = LOG2LL(MIN_ALLOC);

    MAX_ALLOC = (size_t)align_up_pow2(
                    (size_t)PAGE_ALIGN_UP(usable_mem_end - usable_mem_start)) >>
                PAGE_SHIFT;

    MAX_ALLOC_LOG2 = LOG2LL(MAX_ALLOC);

#if defined(DEBUG)
    uart_printf("base_ptr: 0x%x\n", base_ptr);

    uart_printf("max_ptr: 0x%x\n", max_ptr);

    uart_printf("MIN_ALLOC_LOG2: %d\n", MIN_ALLOC_LOG2);
    uart_printf("MAX_ALLOC_LOG2: %d\n", MAX_ALLOC_LOG2);
#endif

    node_is_split = (uint8_t*)mem_alloc(NODE_IS_SPLIT_SIZE * sizeof(uint8_t));

    INIT_NODE_IS_SPLIT();


    node_is_reserved =
        (uint8_t*)mem_alloc(NODE_IS_RESERVED_SIZE * sizeof(uint8_t));

    INIT_NODE_IS_RESERVED();

    free_list =
        (struct list_head**)mem_alloc(BUCKET_COUNT * sizeof(struct list_head*));

    for (int i = 0; i < BUCKET_COUNT; i++) {
        free_list[i] = (struct list_head*)mem_alloc(sizeof(struct list_head));
        INIT_LIST_HEAD(free_list[i]);
    }

    register_reserve_pages(DTS_MEM_RESERVED_START, DTS_MEM_RESERVED_END);
    register_reserve_pages(dtb_start, dtb_end);
    register_reserve_pages(cpio_start, cpio_end);
    register_reserve_pages((uintptr_t)&kernel_start, (uintptr_t)&kernel_end);
    register_reserve_pages(usable_mem_end,
                           usable_mem_start + (MAX_ALLOC << PAGE_SHIFT));

    start_init_pages();

    print_free_list();
}

void print_free_list(void)
{
    uart_printf("\n===================================\n");
    uart_printf("Free List layout\n");
    uart_printf("===================================\n");

    for (int i = MIN_ALLOC_LOG2; i < MIN_ALLOC_LOG2 + BUCKET_COUNT; i++) {
        uart_printf("ORDER %d: ", MAX_ALLOC_LOG2 - i);
        uart_printf("HEAD(0x%x) -> ", free_list[i]);
        struct list_head* node;
        list_for_each (node, free_list[i]) {
            uart_printf("0x%x -> ");
        }
        uart_printf("\n");
    }
}

void test_page_alloc(void)
{
    uint8_t* ptr1 = alloc_pages(1 << PAGE_SHIFT);

    print_free_list();

    uint8_t* ptr2 = alloc_pages((1 << 5) << PAGE_SHIFT);

    print_free_list();

    free_pages(ptr1);

    print_free_list();

    uint8_t* ptr3 = alloc_pages((1 << 8) << PAGE_SHIFT);

    print_free_list();

    free_pages(ptr3);

    print_free_list();

    uint8_t* ptr4 = alloc_pages(0);

    print_free_list();

    free_pages(ptr2);

    print_free_list();

    free_pages(ptr4);

    print_free_list();
}
