#ifndef SLAB_H
#define SLAB_H

#include "def.h"
#include "int.h"
#include "list.h"

#define OO_SHIFT 16
#define OO_MASK  ((1 << OO_SHIFT) - 1)

#define KMALLOC_SHIFT_HIGH     (PAGE_SHIFT - 1)
#define KMALLOC_SHIFT_LOW      3
#define KMALLOC_MIN_SIZE       (1 << KMALLOC_SHIFT_LOW)
#define KMALLOC_MAX_CACHE_SIZE (1 << KMALLOC_SHIFT_HIGH)

struct kmem_cache_order_objects {
    /*
     * 31        15        0
     * <-- 16 --> <-- 16 -->
     *    order   nr_objects
     */
    uint32_t x;
};

struct kmem_cache {
    struct list_head list;  // list of slab caches
    uint32_t size;          // the size of an object including metadata
    uint32_t object_size;   // the size of an object without metadata
    uint32_t inuse;         // the size of an object with word-alignment
    uint32_t offset;        // free pointer offset

    struct kmem_cache_order_objects oo;
    struct kmem_cache_order_objects min;

    uint64_t min_partial;

    int32_t refcount;

    uint32_t align;
    const char* name;

    struct kmem_cache_node* node;
};

struct kmem_cache_node {
    uint64_t nr_partial;
    struct list_head partial;
};


struct kmalloc_info_struct {
    const char* name;
    uint32_t size;
};

void kmem_cache_init(void);
struct kmem_cache* kmem_cache_create(const char* name,
                                     size_t size,
                                     size_t align);
void kmem_cache_destroy(struct kmem_cache* cachep);
void* kmem_cache_alloc(struct kmem_cache* cachep);
void kmem_cache_free(struct kmem_cache* cachep, void* objp);

void* kmalloc(size_t size);
void kfree(const void* x);

void slabinfo(void);

#endif /* SLAB_H */
