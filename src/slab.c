#include "slab.h"
#include "mm.h"
#include "kernel.h"

static struct kmem_cache kmem_caches[NR_OBJECTS];
struct kmem_cache *kmem_cache = kmem_caches;

/* Init the kmem_cache_node structure. */
static inline void kmem_cache_node_init(struct kmem_cache_node *kmem_cache_node)
{
    kmem_cache_node->nr_partial = 0;
    INIT_LIST_HEAD(&kmem_cache_node->partial);
}

/* Calculate the size with meta data according to object size. */
static inline unsigned int calculate_size(unsigned int obj_size)
{
    unsigned int size = ALIGN(obj_size, sizeof(void *));
    return size;
}

static inline void kmem_cache_init(struct kmem_cache *cache, unsigned int object_size)
{
    cache->object_size = object_size;
    cache->size = calculate_size(object_size);
    cache->offset = 0;
    kmem_cache_node_init(&cache->node);
}

/* Init kmem_caches and kmem_cache_node */
static inline void kmem_caches_init(void)
{
    for (int i = 0; i < NR_OBJECTS; i++)
        kmem_cache_init(&kmem_caches[i], OBJECT_SIZE(i));
}

/* Return the physical address of slab */
static inline void *slab_address(struct page *slab)
{
    return (void *) page_to_phys(slab);
}

/* Setup the object value if needed. But not implemented here, just return the address. */
static inline void *setup_object(struct kmem_cache *cache, void *object)
{
    /* We may setup `track` or `red zone` here. But not implement for now. */
    return object;
}

/* Set the free pointer of object. Point to next object. */
static inline void set_freepointer(struct kmem_cache *s, void *object, void *fp)
{
    void *freeptr_addr = (void **) object + s->offset;
    *(void **) freeptr_addr = fp;
}

/* Allocate a new slab for kmem_cache. */
static struct page *allocate_slab(struct kmem_cache *cache)
{
    struct page *slab;
    void *start, *p;
    unsigned int nr_objects, i; // number of objects in a slab

    slab = __alloc_pages(0);
    if (!slab)
        return NULL;

    /* Setup the flags of slab, so we can know where a given address belongs to slab or buddy system by this flag */
    slab->flags = PG_slab;
    
    /* Init slab's list_head and insert into kmem_cache */
    INIT_LIST_HEAD(&slab->slab_list);
    list_add(&slab->slab_list, &cache->node.partial);

    /* Init slab's kmem_cache pointer */
    slab->slab_cache = cache;

    /* Init slab's freelist pointer. Point it to the physical memory. */
    start = slab_address(slab);
    start = setup_object(cache, (void *) start);

    /* Point freelist to start */
    slab->freelist = (void *) start;

    nr_objects = 4096 / cache->size; // page size/object size

    /* Setup the objects in order. */
    for (i = 0, p = start; i < nr_objects - 1; i++)
    {
        void *next = p + cache->size;
        setup_object(cache, next);
        set_freepointer(cache, p, next);
        p = next;
    }
    set_freepointer(cache, p, NULL);

    return slab;
}

static inline void *__get_object(struct kmem_cache *cache)
{
    void *object;
    struct page *slab;

    /* cache->node.partial has the slab structure list_head */
    list_for_each_entry(slab, &cache->node.partial, slab_list) {
        if (slab->freelist) {
            object = slab->freelist;
            slab->freelist = *(void **) object;
            return object;
        }
    }

    /* No object can be allocated from kmem_cache, we allocate a new slab */
    slab = allocate_slab(cache);
    if (!slab)
        return NULL;
    object = slab->freelist;
    slab->freelist = *(void **) object;
    return object;
}

/* the size should be less or equal to 2048 */
void *get_object(unsigned int size)
{
    void *object = NULL;
    int idx;

    /* Decide the object size: 8, 16, 32.... */
    for (idx = 0; idx < NR_OBJECTS; idx++)
        if (size <= OBJECT_SIZE(idx))
            break;

    /* Allocate the obj_size slab from the kmem_cache */
    object = __get_object(&kmem_caches[idx]);

    /* Return the physiacl memory address of the obj */
    return object;
}

void free_object(void *object)
{
    struct kmem_cache *cache;
    struct page *slab;

    /* Use object physical address to get the slab and slab cache*/
    slab = phys_to_page(object);

    /* Check whether the page is a slab */
    if (slab->flags != PG_slab)
        return;

    cache = slab->slab_cache;

    /* Add the object to the freelist */
    set_freepointer(cache, object, slab->freelist);
    slab->freelist = object;
}

/* Init slab allocator. Setup all the kmem_cache. */
void slab_init()
{
    kmem_caches_init();
}