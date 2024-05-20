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

#define ALIGN(x, a) (((x) + ((x) & ((a) - 1))) & ~((a) - 1))
#define MIN_ALIGN   8
#define MAX_ALIGN   (1 << 8)

static struct list_head slab_caches;
static struct kmem_cache boot_kmem_cache, boot_kmem_cache_node;
struct kmem_cache* kmalloc_caches[KMALLOC_SHIFT_HIGH + 1];

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

static uint32_t order_objects(uint32_t order, uint32_t size)
{
    return (PAGE_SIZE << order) / size;
}

static struct kmem_cache_order_objects oo_make(uint32_t order, uint32_t size)
{
    struct kmem_cache_order_objects oo = {.x = (order << OO_SHIFT) +
                                               order_objects(order, size)};
    return oo;
}

static uint16_t oo_order(struct kmem_cache_order_objects oo)
{
    return oo.x >> OO_SHIFT;
}

static uint16_t oo_objects(struct kmem_cache_order_objects oo)
{
    return oo.x & OO_MASK;
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


    uint16_t order = oo_order(slab_cache->oo);
    gfp_t flags = 0;

    if (order)
        flags |= __GFP_COMP;

    struct page* new_slab = alloc_pages(order, flags);

    if (!new_slab) {
        order = oo_order(slab_cache->min);
        flags = 0;
        if (order)
            flags |= __GFP_COMP;
        new_slab = alloc_pages(order, flags);
    }

    if (!new_slab)
        return 0;

    SetPageSlab(new_slab);


    new_slab->freelist = (void*)ALIGN((uintptr_t)new_slab + sizeof(struct page),
                                      slab_cache->align);

    new_slab->objects = oo_objects(slab_cache->oo);
    new_slab->inuse = 0;
    new_slab->frozen = 0;

    struct page* page = new_slab;
    uint8_t* page_end = (uint8_t*)page + PAGE_SIZE;
    void *p, *next;
    void* start = new_slab->freelist;

    size_t nr_pages = 1 << order;
    while (nr_pages--) {
        p = start;
        next = start + slab_cache->size;

        for (; (uint8_t*)next < page_end; p = next, next += slab_cache->size)
            set_freepointer(slab_cache, p, next);

        page = (struct page*)page_end;
        page_end += PAGE_SIZE;
        start = (nr_pages == 1)
                    ? NULL
                    : (void*)ALIGN((uintptr_t)page + sizeof(struct page),
                                   slab_cache->align);
        set_freepointer(slab_cache, p, start);
    }



    new_slab->slab_cache = slab_cache;
    list_add(&new_slab->slab_list, &slab_cache->node->partial);
    slab_cache->node->nr_partial++;

    return 1;
}

static void create_boot_cache(struct kmem_cache* s,
                              const char* name,
                              uint32_t size)
{
    s->name = name;
    s->object_size = size;
    s->align = 8;
    s->refcount = -1;
    s->offset = 0;

    INIT_LIST_HEAD(&s->list);
    s->inuse = ALIGN(size, sizeof(void*));
    s->size = ALIGN(s->inuse, s->align);

    s->oo = oo_make(0, s->size);
    s->min = oo_make(0, s->size);

    s->min_partial = 10;

    s->node = mem_alloc(sizeof(struct kmem_cache_node));
    s->node->nr_partial = 0;
    INIT_LIST_HEAD(&s->node->partial);
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
        mem_set(object_addr, 0, cachep->object_size);

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

    struct page* page = get_page_from_ptr(objp);
    if (PageCompound(page))
        page = get_compound_head(page);

    if (page->slab_cache != cachep)
        return;

    set_freepointer(cachep, objp, page->freelist);
    page->freelist = objp;
    page->inuse--;


    /* slab empty */
    if (page->inuse == 0 && cachep->node->nr_partial >= cachep->min_partial) {
        list_del_init(&page->slab_list);
        free_pages(page, oo_order(cachep->oo));
        cachep->node->nr_partial--;
    }
}


struct kmem_cache* kmem_cache_create(const char* name,
                                     size_t size,
                                     size_t align)
{
    if ((int32_t)align < MIN_ALIGN)
        align = MIN_ALIGN;
    else if ((int32_t)align > MAX_ALIGN)
        align = MAX_ALIGN;

    struct kmem_cache* s = kmem_cache_zalloc(&boot_kmem_cache, 0);

    if (!s)
        return NULL;

    INIT_LIST_HEAD(&s->list);

    list_add(&s->list, &slab_caches);

    s->name = name;

    s->object_size = size;
    s->align = align;
    s->offset = 0;

    size = ALIGN(size, sizeof(void*));
    s->inuse = size;

    s->size = ALIGN(size, align);

    s->refcount = -1;

    s->min_partial = 10;

    s->oo = oo_make(0, s->size);
    s->min = oo_make(0, s->size);

    s->node = kmem_cache_zalloc(&boot_kmem_cache_node, 0);

    if (!s->node) {
        kmem_cache_free(&boot_kmem_cache, s);
        return NULL;
    }

    INIT_LIST_HEAD(&s->node->partial);
    s->node->nr_partial = 0;

    return s;
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
        list_del_init(&slab->slab_list);
        cachep->node->nr_partial--;
        free_pages(slab, oo_order(cachep->oo));
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


void slabinfo(void)
{
    struct list_head* node;
    list_for_each (node, &slab_caches) {
        struct kmem_cache* s = list_entry(node, struct kmem_cache, list);
        uart_printf("%s -> ", s->name);
    }
    uart_printf("\n");
}

static inline void* kmalloc_large(size_t size)
{
    uint32_t order = order_for_request(size + sizeof(struct page));
    return (void*)(alloc_pages(order, 0) + 1);
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
    struct page* page;
    void* object = (void*)x;

    if (!x)
        return;

    page = get_page_from_ptr(object);

    bool compound = PageCompound(page);
    if (compound)
        page = get_compound_head(page);

    // if it's buddy system allocated.
    if (!PageSlab(page)) {
        size_t order = compound ? page->compound_order : page->private;
        free_pages(page, order);
        return;
    }

    kmem_cache_free(page->slab_cache, object);
}

void kmem_cache_init(void)
{
    create_boot_cache(&boot_kmem_cache_node, "kmem_cache_node",
                      sizeof(struct kmem_cache_node));
    create_boot_cache(&boot_kmem_cache, "kmem_cache",
                      sizeof(struct kmem_cache));

    INIT_LIST_HEAD(&slab_caches);
    list_add(&boot_kmem_cache.list, &slab_caches);
    list_add(&boot_kmem_cache_node.list, &slab_caches);

    setup_kmalloc_cache_index_table();
    create_kmalloc_caches();
}
