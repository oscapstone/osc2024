#include "slab.h"
#include "array.h"
#include "bool.h"
#include "build_bug.h"
#include "list.h"
#include "memory.h"
#include "mini_uart.h"
#include "page_alloc.h"
#include "page_flags.h"
#include "string.h"

extern size_t MAX_ORDER;

#define MIN_PARTIAL 5
#define MAX_PARTIAL 10

#define MAX_OBJS_PER_PAGE 32767  // since page.objects is u15

#define OO_SHIFT 16
#define OO_MASK  ((1 << OO_SHIFT) - 1)

#define KMALLOC_SHIFT_HIGH (PAGE_SHIFT + 1)
#define KMALLOC_SHIFT_LOW  3
#define KMALLOC_SHIFT_MAX  (MAX_ORDER + PAGE_SHIFT - 1)

/* Maximum allocatable size */
#define KMALLOC_MAX_SIZE (1UL << KMALLOC_SHIFT_MAX)
/* Maximum size for which we can actually use a slab cache */
#define KMALLOC_MAX_CACHE_SIZE (1UL << KMALLOC_SHIFT_HIGH)
/* Maximum order allocatable via the slab allocator */
#define KMALLOC_MAX_ORDER (KMALLOC_SHIFT_MAX - PAGE_SHIFT)


#define KMALLOC_MIN_SIZE (1UL << KMALLOC_SHIFT_LOW)

#define PAGE_ALLOC_COSTLY_ORDER 3

#define MIN_ALIGN 8
#define MAX_ALIGN (1 << 8)

static struct list_head slab_caches;
static struct kmem_cache boot_kmem_cache, boot_kmem_cache_node;
struct kmem_cache* kmalloc_caches[KMALLOC_SHIFT_HIGH + 1];

static unsigned int slub_min_order;
static unsigned int slub_max_order = PAGE_ALLOC_COSTLY_ORDER;
static unsigned int slub_min_objects;

const struct kmalloc_info_struct kmalloc_info[] = {
    {NULL, 0},
    {"kmalloc-96", 96},
    {"kmalloc-192", 192},
    {"kmalloc-8", 8},
    {"kmalloc-16", 16},
    {"kmalloc-32", 32},
    {"kmalloc-64", 64},
    {"kmalloc-128", 128},
    {"kmalloc-256", 256},
    {"kmalloc-512", 512},
    {"kmalloc-1k", 1024},
    {"kmalloc-2k", 2048},
    {"kmalloc-4k", 4096},
    {"kmalloc-8k", 8192},
    {"kmalloc-16k", 16384},
    {"kmalloc-32k", 32768},
    {"kmalloc-64k", 65536},
    {"kmalloc-128k", 131072},
    {"kmalloc-256k", 262144},
    {"kmalloc-512k", 524288},
    {"kmalloc-1M", 1048576},
    {"kmalloc-2M", 2097152},
    {"kmalloc-4M", 4194304},
    {"kmalloc-8M", 8388608},
    {"kmalloc-16M", 16777216},
    {"kmalloc-32M", 33554432},
    {"kmalloc-64M", 67108864},
};

static uint8_t size_index[24] = {
    3, /* 8 */
    4, /* 16 */
    5, /* 24 */
    5, /* 32 */
    6, /* 40 */
    6, /* 48 */
    6, /* 56 */
    6, /* 64 */
    1, /* 72 */
    1, /* 80 */
    1, /* 88 */
    1, /* 96 */
    7, /* 104 */
    7, /* 112 */
    7, /* 120 */
    7, /* 128 */
    2, /* 136 */
    2, /* 144 */
    2, /* 152 */
    2, /* 160 */
    2, /* 168 */
    2, /* 176 */
    2, /* 184 */
    2  /* 192 */
};

static inline uint32_t order_objects(uint32_t order, uint32_t size)
{
    return (PAGE_SIZE << order) / size;
}

static inline struct kmem_cache_order_objects oo_make(uint32_t order,
                                                      uint32_t size)
{
    struct kmem_cache_order_objects oo = {.x = (order << OO_SHIFT) +
                                               order_objects(order, size)};
    return oo;
}

static inline uint16_t oo_order(struct kmem_cache_order_objects oo)
{
    return oo.x >> OO_SHIFT;
}

static inline uint16_t oo_objects(struct kmem_cache_order_objects oo)
{
    return oo.x & OO_MASK;
}

static inline unsigned int slab_order(unsigned int size,
                                      unsigned int min_objects,
                                      unsigned int max_order,
                                      unsigned int fract_leftover)
{
    unsigned int min_order = slub_min_order;
    unsigned int order;

    if (order_objects(min_order, size) > MAX_OBJS_PER_PAGE)
        return get_order(size * MAX_OBJS_PER_PAGE) - 1;

    for (order = max(min_order, (unsigned int)get_order(min_objects * size));
         order <= max_order; order++) {
        unsigned int slab_size = (unsigned int)PAGE_SIZE << order;
        unsigned int rem;
        rem = slab_size % size;
        if (rem <= slab_size / fract_leftover)
            break;
    }

    return order;
}

static inline int calculate_order(unsigned int size)
{
    unsigned int order;
    unsigned int min_objects;
    unsigned int max_objects;

    min_objects = slub_min_objects;
    if (!min_objects)
        min_objects = 4;

    max_objects = order_objects(slub_max_order, size);
    min_objects = min(min_objects, max_objects);

    while (min_objects > 1) {
        unsigned int fraction;

        fraction = 16;
        while (fraction >= 4) {
            order = slab_order(size, min_objects, slub_max_order, fraction);
            if (order <= slub_max_order)
                return order;
            fraction /= 2;
        }
        min_objects--;
    }

    order = slab_order(size, 1, MAX_ORDER, 1);
    if (order < MAX_ORDER)
        return order;

    return -1;
}

static int calculate_sizes(struct kmem_cache* s, int forced_order)
{
    unsigned int size = s->object_size;
    unsigned int order;

    size = ALIGN(size, sizeof(void*));
    s->inuse = size;
    size = ALIGN(size, s->align);
    s->size = size;

    if (forced_order >= 0)
        order = forced_order;
    else
        order = calculate_order(size);

    if ((int)order < 0)
        return 0;

    s->allocflags = 0;

    if (order)
        s->allocflags |= __GFP_COMP;

    s->oo = oo_make(order, size);
    s->min = oo_make(get_order(size), size);

    if (oo_objects(s->oo) > oo_objects(s->max))
        s->max = s->oo;

    return !!oo_objects(s->oo);
}

static inline void set_freepointer(struct kmem_cache* s, void* object, void* fp)
{
    uintptr_t freeptr_addr = (uintptr_t)object + s->offset;
    *(uintptr_t*)freeptr_addr = (uintptr_t)fp;
}

static inline void* get_freepointer(struct kmem_cache* s, void* object)
{
    return (void*)(*(uintptr_t*)((uint8_t*)object + s->offset));
}

static int allocate_slab(struct kmem_cache* slab_cache)
{
    if (!slab_cache)
        return 0;

    struct kmem_cache_order_objects oo = slab_cache->oo;

    uint16_t order = oo_order(oo);
    gfp_t flags = slab_cache->allocflags;

    struct page* new_slab = alloc_pages(order, flags);

    if (!new_slab) {
        oo = slab_cache->min;
        order = oo_order(oo);
        new_slab = alloc_pages(order, flags);
    }

    if (!new_slab)
        return 0;

    SetPageSlab(new_slab);

    new_slab->freelist = page_to_phys(new_slab);

    new_slab->objects = oo_objects(oo);
    new_slab->inuse = 0;
    new_slab->frozen = 0;

    int i;
    void *p, *next;
    for (i = 0, p = new_slab->freelist; i < new_slab->objects - 1; i++) {
        next = p + slab_cache->size;
        set_freepointer(slab_cache, p, next);
        p = next;
    }
    set_freepointer(slab_cache, p, NULL);

    new_slab->slab_cache = slab_cache;

    INIT_LIST_HEAD(&new_slab->slab_list);
    list_add(&new_slab->slab_list, &slab_cache->node->partial);
    slab_cache->node->nr_partial++;

    return 1;
}

static void free_slab(struct kmem_cache* slab_cache, struct page* slab)
{
    if (slab->slab_cache != slab_cache)
        return;

    size_t order = get_compound_order(slab);

    list_del_init(&slab->slab_list);
    ClearPageSlab(slab);
    free_pages(slab, order);
    slab_cache->node->nr_partial--;
}

static int init_kmem_cache_nodes(struct kmem_cache* s, bool boot)
{
    if (boot)
        s->node = mem_alloc(sizeof(struct kmem_cache_node));
    else
        s->node = kmem_cache_zalloc(&boot_kmem_cache_node, 0);

    if (!s->node)
        return 0;

    s->node->nr_partial = 0;
    INIT_LIST_HEAD(&s->node->partial);

    return 1;
}

static void create_boot_cache(struct kmem_cache* s,
                              const char* name,
                              uint32_t size)
{
    if (!s)
        return;

    s->name = name;
    s->object_size = size;
    s->align = MIN_ALIGN;
    s->refcount = -1;
    s->offset = 0;
    s->min_partial = MIN_PARTIAL;

    INIT_LIST_HEAD(&s->list);

    if (!calculate_sizes(s, -1))
        return;


    if (!init_kmem_cache_nodes(s, 1))
        return;

    list_add(&s->list, &slab_caches);
}

void* kmem_cache_alloc(struct kmem_cache* cachep, gfp_t flags)
{
    if (!cachep)
        return NULL;

    struct page* slab;
    struct list_head* node;
    void* object_addr;
    list_for_each (node, &cachep->node->partial) {
        slab = list_entry(node, struct page, slab_list);
        if (slab->freelist)
            goto found_object;
    }

    // No slabs or None of the slabs have free slot
    if (!allocate_slab(cachep))
        return NULL;

    slab = list_first_entry(&cachep->node->partial, struct page, slab_list);

found_object:
    object_addr = slab->freelist;
    slab->freelist = get_freepointer(cachep, object_addr);
    slab->inuse++;

    if (flags & __GFP_ZERO)
        memset(object_addr, 0, cachep->object_size);

    return object_addr;
}

void* kmem_cache_zalloc(struct kmem_cache* cachep, gfp_t flags)
{
    return kmem_cache_alloc(cachep, flags | __GFP_ZERO);
}

void kmem_cache_free(struct kmem_cache* cachep, void* objp)
{
    if (!cachep || !objp)
        return;

    struct page* page = phys_to_page(PAGE_ALIGN_DOWN(objp));
    if (PageTail(page))
        page = get_compound_head(page);

    if (page->slab_cache != cachep)
        return;

    set_freepointer(cachep, objp, page->freelist);
    page->freelist = objp;
    page->inuse--;


    /* slab empty */
    if (page->inuse == 0 && cachep->node->nr_partial >= cachep->min_partial)
        free_slab(cachep, page);
}


struct kmem_cache* kmem_cache_create(const char* name,
                                     size_t size,
                                     size_t align)
{
    if (!name)
        goto error;

    if ((int32_t)align < MIN_ALIGN)
        align = MIN_ALIGN;
    else if ((int32_t)align > MAX_ALIGN)
        align = MAX_ALIGN;

    struct kmem_cache* s = kmem_cache_zalloc(&boot_kmem_cache, 0);

    if (!s)
        goto error;

    INIT_LIST_HEAD(&s->list);

    s->name = name;

    s->object_size = size;
    s->align = align;
    s->offset = 0;

    s->refcount = -1;
    s->min_partial = MIN_PARTIAL;


    if (!calculate_sizes(s, -1))
        goto error;


    if (!init_kmem_cache_nodes(s, 0))
        goto error;

    list_add(&s->list, &slab_caches);

    return s;

error:
    if (s)
        kmem_cache_free(&boot_kmem_cache, s);
    return NULL;
}

void kmem_cache_destroy(struct kmem_cache* cachep)
{
    struct list_head *node, *safe;
    struct page* slab;

    // check if all the objects is freed
    // if not, we can't destroy this cache
    list_for_each_safe (node, safe, &cachep->node->partial) {
        slab = list_entry(node, struct page, slab_list);
        if (slab->inuse != 0)
            return;
        free_slab(cachep, slab);
    }

    if (list_empty(&cachep->node->partial)) {
        kmem_cache_free(&boot_kmem_cache_node, cachep->node);
        cachep->node = NULL;
    }

    if (cachep->node == NULL) {
        list_del_init(&cachep->list);
        kmem_cache_free(&boot_kmem_cache, cachep);
    }
}

static inline void* kmalloc_large(size_t size)
{
    size_t order = get_order(size);
    struct page* page = alloc_pages(order, __GFP_COMP);
    return page_to_phys(page);
}

static inline uint32_t kmalloc_index(size_t size)
{
    if (!size)
        return 0;
    if (size <= KMALLOC_MIN_SIZE)
        return KMALLOC_SHIFT_LOW;
    if (KMALLOC_MIN_SIZE <= 32 && size > 64 && size <= 96)
        return 1;
    if (KMALLOC_MIN_SIZE <= 64 && size > 128 && size <= 192)
        return 2;
    if (size <= 8)
        return 3;
    if (size <= 16)
        return 4;
    if (size <= 32)
        return 5;
    if (size <= 64)
        return 6;
    if (size <= 128)
        return 7;
    if (size <= 256)
        return 8;
    if (size <= 512)
        return 9;
    if (size <= 1024)
        return 10;
    if (size <= 2 * 1024)
        return 11;
    if (size <= 4 * 1024)
        return 12;
    if (size <= 8 * 1024)
        return 13;
    if (size <= 16 * 1024)
        return 14;
    if (size <= 32 * 1024)
        return 15;
    if (size <= 64 * 1024)
        return 16;
    if (size <= 128 * 1024)
        return 17;
    if (size <= 256 * 1024)
        return 18;
    if (size <= 512 * 1024)
        return 19;
    if (size <= 1024 * 1024)
        return 20;
    if (size <= 2 * 1024 * 1024)
        return 21;
    if (size <= 4 * 1024 * 1024)
        return 22;
    if (size <= 8 * 1024 * 1024)
        return 23;
    if (size <= 16 * 1024 * 1024)
        return 24;
    if (size <= 32 * 1024 * 1024)
        return 25;
    if (size <= 64 * 1024 * 1024)
        return 26;

    // should not get here
    return -1;
}

void* kmalloc(size_t size, gfp_t flags)
{
    uint32_t index;

    if (size > KMALLOC_MAX_SIZE)
        return NULL;

    if (size > KMALLOC_MAX_CACHE_SIZE)
        return kmalloc_large(size);

    index = kmalloc_index(size);

    if (!index)
        return NULL;

    return kmem_cache_alloc(kmalloc_caches[index], flags);
}

void* kzmalloc(size_t size, gfp_t flags)
{
    return kmalloc(size, flags | __GFP_ZERO);
}

static inline uint32_t size_index_elem(uint32_t bytes)
{
    return (bytes - 1) / 8;
}

void setup_kmalloc_cache_index_table(void)
{
    uint32_t i;
    BUILD_BUG_ON(KMALLOC_MIN_SIZE > 256 ||
                 (KMALLOC_MIN_SIZE & (KMALLOC_MIN_SIZE - 1)));

    for (i = 8; i < KMALLOC_MIN_SIZE; i += 8) {
        uint32_t elem = size_index_elem(i);
        if (elem >= ARRAY_SIZE(size_index))
            break;

        size_index[elem] = KMALLOC_SHIFT_LOW;
    }

    if (KMALLOC_MIN_SIZE >= 64) {
        for (i = 64 + 8; i <= 96; i += 8)
            size_index[size_index_elem(i)] = 7;
    }

    if (KMALLOC_MIN_SIZE >= 128) {
        for (i = 128 + 8; i <= 192; i += 8)
            size_index[size_index_elem(i)] = 8;
    }
}

struct kmem_cache* create_kmalloc_cache(const char* name, uint32_t size)
{
    struct kmem_cache* s = kmem_cache_create(name, size, -1);
    if (!s)
        return NULL;

    s->refcount = 1;
    return s;
}


static void new_kmalloc_cache(int idx)
{
    const char* name = kmalloc_info[idx].name;
    kmalloc_caches[idx] = create_kmalloc_cache(name, kmalloc_info[idx].size);
}

void create_kmalloc_caches(void)
{
    int i;
    for (i = KMALLOC_SHIFT_LOW; i <= KMALLOC_SHIFT_HIGH; i++) {
        if (!kmalloc_caches[i])
            new_kmalloc_cache(i);

        if (KMALLOC_MIN_SIZE <= 32 && i == 6 && !kmalloc_caches[1])
            new_kmalloc_cache(1);

        if (KMALLOC_MIN_SIZE <= 64 && i == 7 && !kmalloc_caches[2])
            new_kmalloc_cache(2);
    }
}

void kfree(const void* x)
{
    if (!x)
        return;

    struct page* page;
    void* object = (void*)x;

    page = phys_to_page(PAGE_ALIGN_DOWN(object));

    if (PageTail(page))
        page = get_compound_head(page);

    // if it's buddy system allocated.
    if (!PageSlab(page)) {
        size_t order = get_compound_order(page);
        free_pages(page, order);
        return;
    }

    // otherwise, it's slab cache allocated.
    kmem_cache_free(page->slab_cache, object);
}

void kmem_cache_init(void)
{
    INIT_LIST_HEAD(&slab_caches);

    create_boot_cache(&boot_kmem_cache_node, "kmem_cache_node",
                      sizeof(struct kmem_cache_node));

    create_boot_cache(&boot_kmem_cache, "kmem_cache",
                      sizeof(struct kmem_cache));

    setup_kmalloc_cache_index_table();

    create_kmalloc_caches();
}

void slabinfo(void)
{
    uart_printf("\n===================================\n");
    uart_printf("Slab Info\n");
    uart_printf("===================================\n");

    uart_printf("\n| Name | #Slab | #inuse / #objects |\n");
    struct kmem_cache* cache;
    struct page* slab;
    list_for_each_entry (cache, &slab_caches, list) {
        size_t objects = 0;
        size_t inuse = 0;
        list_for_each_entry (slab, &cache->node->partial, slab_list) {
            objects += slab->objects;
            inuse += slab->inuse;
        }
        uart_printf("| %s | %d | %d / %d |\n", cache->name,
                    cache->node->nr_partial, inuse, objects);
    }
    uart_printf("\n");
}

void test_slab_alloc(void)
{
    uart_printf("\n==========================\n");
    uart_printf("Create new slab cache\n");
    struct kmem_cache* fuck = kmem_cache_create("fuck", 10, -1);
    slabinfo();
    uart_printf("\n==========================\n");

    uart_printf("\n==========================\n");
    uart_printf("Allocate object from new slab cache\n");
    void* ptr = kmem_cache_alloc(fuck, 0);
    uart_printf("allocated object at 0x%x\n", ptr);
    slabinfo();
    uart_printf("\n==========================\n");


    uart_printf("\n==========================\n");
    uart_printf("Free object from new slab cache\n");
    kmem_cache_free(fuck, ptr);
    slabinfo();
    uart_printf("\n==========================\n");


    uart_printf("\n==========================\n");
    uart_printf("Delete the new slab cache\n");
    kmem_cache_destroy(fuck);
    slabinfo();
    uart_printf("\n==========================\n");


    uart_printf("\n==========================\n");
    uart_printf("kmalloc 40B\n");
    void* ptr1 = kmalloc(10 * sizeof(int), 0);
    uart_printf("allocated object at 0x%x\n", ptr1);
    slabinfo();
    uart_printf("\n==========================\n");

    uart_printf("\n==========================\n");
    uart_printf("kfree 40B\n");
    kfree(ptr1);
    slabinfo();
    uart_printf("\n==========================\n");

    uart_printf("\n==========================\n");
    uart_printf("kmalloc 4KB\n");
    void* ptr2 = kmalloc(4 * 1024, 0);
    uart_printf("allocated object at 0x%x\n", ptr2);
    slabinfo();
    uart_printf("\n==========================\n");

    uart_printf("\n==========================\n");
    uart_printf("kfree 4KB\n");
    kfree(ptr2);
    slabinfo();
    uart_printf("\n==========================\n");

    uart_printf("\n==========================\n");
    uart_printf("kmalloc 10KB\n");
    void* ptr3 = kmalloc(10 * 1024, 0);
    uart_printf("allocated object at 0x%x\n", ptr3);
    slabinfo();
    uart_printf("\n==========================\n");

    uart_printf("\n==========================\n");
    uart_printf("kfree 10KB\n");
    kfree(ptr3);
    slabinfo();
    uart_printf("\n==========================\n");
}
