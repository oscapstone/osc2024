#include "page_alloc.h"
#include "bool.h"
#include "cpio.h"
#include "dtb.h"
#include "math.h"
#include "memory.h"
#include "mini_uart.h"
#include "page_flags.h"

static struct zone zone;
static struct free_area* free_areas;

// frame array
static struct page* mem_map;


static uintptr_t base_ptr, max_ptr;

extern char kernel_start[];
extern char kernel_end[];

static uintptr_t usable_mem_end;

/* reserved memory region - spin table */
#define DTS_MEM_RESERVED_START  0x0 + VA_START
#define DTS_MEM_RESERVED_LENGTH 0x1000
#define DTS_MEM_RESERVED_END    (DTS_MEM_RESERVED_START + DTS_MEM_RESERVED_LENGTH)


size_t MAX_ORDER, MIN_ORDER;
size_t MAX_ALLOC_SIZE, MIN_ALLOC_SIZE;

#define BUCKET_COUNT (MAX_ORDER - MIN_ORDER + 1)

#define get_order_from_bucket(__bucket) ((__bucket) + MIN_ORDER)
#define get_bucket_from_order(__order)  ((__order) - MIN_ORDER)

inline void* PAGE_ALIGN_DOWN(void* x)
{
    return (void*)((((uintptr_t)x - base_ptr) & ~(PAGE_SIZE - 1)) + base_ptr);
}

inline void* PAGE_ALIGN_UP(void* x)
{
    return (void*)((uintptr_t)mem_align((void*)((uintptr_t)x - base_ptr),
                                        PAGE_SIZE) +
                   base_ptr);
}

inline void* pfn_to_phys(size_t pfn)
{
    return (void*)((pfn << PAGE_SHIFT) + base_ptr);
}

inline size_t phys_to_pfn(void* phys)
{
    return ((uintptr_t)phys - base_ptr) >> PAGE_SHIFT;
}

inline struct page* pfn_to_page(size_t pfn)
{
    return &mem_map[pfn];
}

inline size_t page_to_pfn(struct page* page)
{
    return page - mem_map;
}

inline void* page_to_phys(struct page* page)
{
    return pfn_to_phys(page_to_pfn(page));
}

inline struct page* phys_to_page(void* phys)
{
    return pfn_to_page(phys_to_pfn(phys));
}

#define get_buddy_pfn(__pfn, __order) ((__pfn) ^ (1 << (__order)))
#define get_left_pfn(__pfn, __order)  ((__pfn) & ~(1 << (__order)))

size_t get_order(size_t size)
{
    size = ALIGN(size, PAGE_SIZE);
    size_t nr_pages = size >> PAGE_SHIFT;
    nr_pages = align_up_pow2(nr_pages);
    if (nr_pages == 0)
        return 0;
    size_t order = LOG2LL(nr_pages);
    return order;
}

static void add_page_to_freelist(struct page* page, size_t order)
{
    if (order < MIN_ORDER || order > MAX_ORDER)
        return;

    size_t bucket = get_bucket_from_order(order);

    list_add_tail(&page->list, &free_areas[bucket].free_list);

    free_areas[bucket].nr_free++;

    SetPageBuddy(page);

    page->private = order;
}


static void delete_page_from_freelist(struct page* page, size_t order)
{
    if (!page)
        return;
    list_del_init(&page->list);
    ClearPageBuddy(page);
    free_areas[get_bucket_from_order(order)].nr_free--;
}


void coalesce(struct page* page, size_t order)
{
#ifdef DEBUG
    uart_printf("  Try to find the buddy to merge...\n");
#endif
    size_t pfn = page_to_pfn(page);

    if (pfn >= zone.managed_pages)
        return;

    while (order <= MAX_ORDER) {
        size_t buddy_pfn = get_buddy_pfn(pfn, order);
        if (buddy_pfn >= zone.managed_pages)
            break;
        struct page* buddy_page = pfn_to_page(buddy_pfn);
#ifdef DEBUG
        uart_printf("  Found buddy at 0x%x\n", page_to_phys(buddy_page));
#endif
        if (PageBuddy(buddy_page) && buddy_page->private == order) {
#ifdef DEBUG
            uart_printf("  Mergable!!\n, merged order: %d\n", order + 1);
#endif
            delete_page_from_freelist(buddy_page, order);
        } else {
#ifdef DEBUG

            uart_printf("  NOT mergable, stop\n");
#endif
            break;
        }
        pfn = get_left_pfn(pfn, order);
        page = pfn_to_page(pfn);
        order++;
    }

#ifdef DEBUG
    uart_printf(
        "  Final Merged address: 0x%x, put it into freelist with order "
        "%d\n",
        page_to_phys(page), order);
#endif
    add_page_to_freelist(page, order);
}

static inline size_t pfn_find_order(size_t pfn, size_t nr_pages)
{
    size_t lsb = get_lowest_set_bit(pfn);
    size_t max_order = get_highest_set_bit(nr_pages);
    if (!lsb)
        return max_order;
    size_t order = LOG2LL(lsb);
    return (order <= max_order) ? order : max_order;
}

static void add_continuous_pages_to_freelist(struct page* base_page,
                                             size_t nr_pages)
{
    if (nr_pages > zone.managed_pages ||
        page_to_pfn(base_page) >= zone.managed_pages)
        return;
    struct page* end = base_page + nr_pages;
    while (base_page < end) {
        size_t pfn = page_to_pfn(base_page);
        size_t order = pfn_find_order(pfn, nr_pages);
        add_page_to_freelist(base_page, order);
        base_page += (1 << order);
        nr_pages -= (1 << order);
    }
}

static struct page* get_page_from_freelist(size_t order)
{
    if (order < MIN_ORDER || order > MAX_ORDER)
        return NULL;

    size_t bucket = get_bucket_from_order(order);

    if (list_empty(&free_areas[bucket].free_list))
        return NULL;

    struct list_head* last = free_areas[bucket].free_list.prev;

    if (!last)
        return NULL;

    list_del_init(last);

    free_areas[bucket].nr_free--;

    struct page* page = list_entry(last, struct page, list);

    if (!page)
        return NULL;

    ClearPageBuddy(page);

    return page;
}


void register_reserved_pages(uintptr_t start, uintptr_t end)
{
    if (end <= start || start >= usable_mem_end || end >= usable_mem_end)
        return;

    start = (uintptr_t)PAGE_ALIGN_DOWN((void*)start);
    end = (uintptr_t)PAGE_ALIGN_UP((void*)end);
    size_t start_pfn = phys_to_pfn((void*)start);
    size_t end_pfn = phys_to_pfn((void*)end);

    while (start_pfn != end_pfn) {
        struct page* page = pfn_to_page(start_pfn);
        SetPageReserved(page);
        start_pfn++;
    }
}

void start_init_pages(void)
{
    size_t pfn = 0;
    struct page *base_page, *page;
    base_page = page = NULL;
    size_t nr_pages = 0;
    while (pfn != zone.managed_pages) {
        page = pfn_to_page(pfn);
        if (base_page) {
            if (PageReserved(page))
                goto add_pages;
            else {
                nr_pages++;
                goto update_pfn;
            }
        } else if (!PageReserved(page)) {
            base_page = page;
            nr_pages++;
            goto update_pfn;
        } else {
            goto update_pfn;
        }

    add_pages:
        add_continuous_pages_to_freelist(base_page, nr_pages);
        base_page = NULL;
        nr_pages = 0;

    update_pfn:
        pfn++;
    }

    if (base_page)
        add_continuous_pages_to_freelist(base_page, nr_pages);
}

static inline void set_compound_head(struct page* page, struct page* head)
{
    page->compound_head = (unsigned long)head + 1;
}

static inline void clear_compound_head(struct page* page)
{
    page->compound_head &= 0;
}

inline struct page* get_compound_head(struct page* page)
{
    return (struct page*)(page->compound_head & -2);
}

static inline void set_compound_order(struct page* page, unsigned int order)
{
    page[1].compound_order = order;
}

inline unsigned int get_compound_order(struct page* page)
{
    if (!PageHead(page))
        return 0;
    return page[1].compound_order;
}

static void prep_compound_tail(struct page* head, int tail_idx)
{
    set_compound_head(head + tail_idx, head);
}

static void reset_compound_tail(struct page* head, int tail_idx)
{
    clear_compound_head(head + tail_idx);
}

static void prep_compound_head(struct page* page, unsigned int order)
{
    set_compound_order(page, order);
}

static void reset_compound_head(struct page* page)
{
    set_compound_order(page, 0);
}

void prep_compound_page(struct page* page, unsigned int order)
{
    size_t i;
    size_t nr_pages = 1 << order;
    SetPageHead(page);
    for (i = 1; i < nr_pages; i++)
        prep_compound_tail(page, i);
    prep_compound_head(page, order);
}

void reset_compound_page(struct page* page)
{
    size_t i;
    size_t order = get_compound_order(page);
    size_t nr_pages = 1 << order;
    ClearPageHead(page);
    for (i = 1; i < nr_pages; i++)
        reset_compound_tail(page, i);
    reset_compound_head(page);
}

void buddy_init(void)
{
    uintptr_t dtb_start = get_dtb_start();
    uintptr_t dtb_end = get_dtb_end();

    uart_printf("dtb_start: 0x%x, dtb_end: 0x%x\n", dtb_start, dtb_end);

    uintptr_t cpio_start = get_cpio_start();
    uintptr_t cpio_end = get_cpio_end();

    uart_printf("cpio_start: 0x%x,  cpio_end: 0x%x\n", cpio_start, cpio_end);

    uart_printf("kernel_start: 0x%x, kernel_end: 0x%x\n",
                (uintptr_t)kernel_start, (uintptr_t)kernel_end);

    uintptr_t usable_mem_start = get_usable_mem_start();
    uintptr_t usable_mem_length = get_usable_mem_length();

    usable_mem_end = usable_mem_start + usable_mem_length;

    base_ptr = max_ptr = usable_mem_start;

    size_t nr_pages = (usable_mem_end - usable_mem_start) >> PAGE_SHIFT;

    MIN_ALLOC_SIZE = 1;
    MIN_ORDER = LOG2LL(MIN_ALLOC_SIZE);

    MAX_ALLOC_SIZE = align_down_pow2(nr_pages);

    MAX_ORDER = LOG2LL(MAX_ALLOC_SIZE);

    zone.managed_pages = nr_pages;
    zone.free_areas = mem_alloc(sizeof(struct free_area) * BUCKET_COUNT);
    if (!zone.free_areas)
        return;

    mem_map = (struct page*)mem_alloc_align(sizeof(struct page) * nr_pages,
                                            page_alignment);

    if (!mem_map)
        return;

    free_areas = zone.free_areas;

    for (int i = 0; i < BUCKET_COUNT; i++) {
        INIT_LIST_HEAD(&free_areas[i].free_list);
        free_areas[i].nr_free = 0;
    }

    memset(mem_map, 0, sizeof(struct page) * nr_pages);

    register_reserved_pages(DTS_MEM_RESERVED_START, DTS_MEM_RESERVED_END);
    register_reserved_pages(dtb_start, dtb_end);
    register_reserved_pages(cpio_start, cpio_end);
    register_reserved_pages((uintptr_t)kernel_start, (uintptr_t)kernel_end);
    register_reserved_pages(HEAP_START, HEAP_END);
    register_reserved_pages(STACK_START, STACK_END);

    start_init_pages();
}


struct page* alloc_pages(size_t order, gfp_t flags)
{
#ifdef DEBUG
    uart_printf("Allocate pages with %d order\n", order);
#endif

    size_t original_order = order;

    while (order <= MAX_ORDER) {
#ifdef DEBUG
        uart_printf("  Search in freelist with %d order\n", order);
#endif
        struct page* page = get_page_from_freelist(order);

        if (!page) {
#ifdef DEBUG
            uart_printf(
                "  Cannot find usable pages in freelist with %d order, try "
                "%d "
                "order\n",
                order, order + 1);
#endif
            order++;
            continue;
        }

#ifdef DEBUG
        uart_printf("  Found usable pages in freelist with %d order\n", order);
#endif
        while (order > original_order) {
            struct page* right_half = page + (1 << (order - 1));
#ifdef DEBUG
            uart_printf(
                "  Usable pages order is larger than requested order, "
                "split it "
                "down to request order\n");
            uart_printf(
                "  Put right half of the pages (0x%x) to freelist with %d "
                "order\n",
                page_to_phys(right_half), order - 1);
#endif
            add_page_to_freelist(right_half, order - 1);
            order--;
        }

        if (flags & __GFP_COMP)
            prep_compound_page(page, original_order);


        if (flags & __GFP_ZERO)
            memset((void*)page_to_phys(page), 0, PAGE_SIZE << original_order);

#ifdef DEBUG
        uart_printf("Allocated pages at 0x%x\n", page_to_phys(page));
#endif

        return page;
    }

    return NULL;
}


void* alloc_pages_exact(size_t size, gfp_t flags)
{
    size = ALIGN(size, PAGE_SIZE);
    size_t nr_pages = (size >> PAGE_SHIFT) + !!(size & (PAGE_SIZE - 1));
    size_t order = get_order(size);
    struct page* page = alloc_pages(order, flags & ~__GFP_COMP);
    size_t remain_pages = (1 << order) - nr_pages;
    add_continuous_pages_to_freelist(page + nr_pages, remain_pages);
    return page_to_phys(page);
}

void free_pages(struct page* page, size_t order)
{
#ifdef DEBUG
    uart_printf("free page 0x%x with %d order\n", page_to_phys(page), order);
#endif
    if (!page || PageBuddy(page) || PageTail(page))
        return;

    if (PageCompound(page))
        reset_compound_page(page);

    coalesce(page, order);
}


void free_pages_exact(void* virt, size_t size)
{
    if (!virt)
        return;
    struct page* page = phys_to_page(virt);
    size = ALIGN(size, PAGE_SIZE);
    size_t nr_pages = size >> PAGE_SHIFT;
    add_continuous_pages_to_freelist(page, nr_pages);
}

void buddyinfo(void)
{
    uart_printf("\n===================================\n");
    uart_printf("Buddy Info\n");
    uart_printf("===================================\n");

    for (size_t i = MIN_ORDER; i <= MAX_ORDER; i++) {
        size_t bucket = get_bucket_from_order(i);
        uart_printf("ORDER %d (#free %d): ", i, free_areas[bucket].nr_free);
        struct page* page;
        list_for_each_entry (page, &free_areas[bucket].free_list, list) {
            uart_printf("0x%x -> ", page_to_phys(page));
        }
        uart_printf("\n");
    }
}

void pageinfo(void)
{
    size_t nr_pages = zone.managed_pages;
    size_t pfn = 0;
    struct page *page, *base_page;
    page = base_page = NULL;

    uart_printf("\n===================================\n");
    uart_printf("Page Info\n");
    uart_printf("===================================\n");

    uart_printf("Total # of page: %d\n", nr_pages);

    nr_pages = 0;

    uart_printf("| address | size | status |\n");

    while (pfn < zone.managed_pages) {
        page = pfn_to_page(pfn);

        if (PageReserved(page)) {
            if (!base_page)
                base_page = page;
            else {
                nr_pages++;
                pfn++;
            }
        } else {
            if (base_page) {
                uart_printf("| 0x%x | %dKB | Reserved |\n",
                            page_to_phys(base_page),
                            (nr_pages << PAGE_SHIFT) >> 10);
                nr_pages = 0;
                base_page = NULL;
            }
            if (PageBuddy(page)) {
                uart_printf("| 0x%x | %dKB | Buddy |\n", page_to_phys(page),
                            (PAGE_SIZE << page->private) >> 10);
                pfn += 1 << page->private;
            } else if (PageSlab(page)) {
                size_t order = 0;
                if (PageHead(page))
                    order = get_compound_order(page);
                uart_printf("| 0x%x | %dKB | Slab |\n", page_to_phys(page),
                            (PAGE_SIZE << order) >> 10);
                pfn += 1 << order;
            }
        }
    }

    uart_printf("\n");
}

void test_page_alloc(void)
{
    uart_printf("\n==========================\n");
    uart_printf("Allocate order 1 pages\n");
    struct page* ptr1 = alloc_pages(1, __GFP_COMP);
    buddyinfo();
    uart_printf("\n==========================\n");



    uart_printf("\n==========================\n");
    uart_printf("Allocate order 5 pages\n");
    struct page* ptr2 = alloc_pages(5, __GFP_COMP);
    buddyinfo();
    uart_printf("\n==========================\n");



    uart_printf("\n==========================\n");
    uart_printf("Free order 1 pages\n");
    free_pages(ptr1, 1);
    buddyinfo();
    uart_printf("\n==========================\n");


    uart_printf("\n==========================\n");
    uart_printf("Allocate order 8 pages\n");
    struct page* ptr3 = alloc_pages(8, __GFP_COMP);
    buddyinfo();
    uart_printf("\n==========================\n");


    uart_printf("\n==========================\n");
    uart_printf("Free order 8 pages\n");
    free_pages(ptr3, 8);
    buddyinfo();
    uart_printf("\n==========================\n");


    uart_printf("\n==========================\n");
    uart_printf("Allocate order 0 pages\n");
    struct page* ptr4 = alloc_pages(0, 0);
    buddyinfo();
    uart_printf("\n==========================\n");


    uart_printf("\n==========================\n");
    uart_printf("Free order 5 pages\n");
    free_pages(ptr2, 5);
    buddyinfo();
    uart_printf("\n==========================\n");

    uart_printf("\n==========================\n");
    uart_printf("Free order 0 pages\n");
    free_pages(ptr4, 0);
    buddyinfo();
    uart_printf("\n==========================\n");

    uart_printf("\n==========================\n");
    uart_printf("Allocate exact 5 pages\n");
    void* ptr5 = alloc_pages_exact(5 << PAGE_SHIFT, 0);
    buddyinfo();
    uart_printf("\n==========================\n");


    uart_printf("\n==========================\n");
    uart_printf("Free exact 5 pages\n");
    free_pages_exact(ptr5, 5 << PAGE_SHIFT);
    buddyinfo();
    uart_printf("\n==========================\n");
}
