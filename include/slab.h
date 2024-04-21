/**
 * In linux kernel, the `struct kmem_cache_node` is designed for NUMA. So we simplify the implementation to one node.
 * So every object has one `kmem_cache` structure to describe the slab cache.
 * Every `kmem_cache` has one `kmem_cache_node` to describe the slab cache node. (If we need to implement NUMA,we can add 
 * the `kmem_cache_node` array to the `kmem_cache` structure.)
 * In fact, even I use only one cpu core, I still need both `kmem_cache_cpu` and `kmem_cache_node` to describe the slab cache.
 * But I skip the `kmem_cache_cpu` structure in this implementation.
 * 
 * For simplicity, I want to use an array of `kmem_cache` to describe the slab cache (Linux kernel use linked list).
 * 
*/
#ifndef __SLAB_H__
#define __SLAB_H__

#include "list.h"

#define NR_OBJECTS              (9) // the number of objects in the slab cache.
#define OBJECT_SIZE(n)          (1 << (n + 3)) // the size of the object in the slab cache.

/* Reference the linux/mm/slab.h: kmem_cache_node with CONFIG_SLUB*/
struct kmem_cache_node {
    unsigned long nr_partial;
    struct list_head partial;
};

/**
 * Slab cache. Describe the slab attibute and link to slabs.
 * Ref: linux/slub_def.h
*/
struct kmem_cache {
    unsigned int size; // object size with meta data
    unsigned int object_size; // object size without meta data
    unsigned int offset; // the offset of the free pointer in the object.
    struct kmem_cache_node node;
};

/* for pointer usage inside the slab (page), the object size starts from 8 byte. */
enum objects{
    OBJECT_8B = 0,
    OBJECT_16B,
    OBJECT_32B,
    OBJECT_64B,
    OBJECT_128B,
    OBJECT_256B,
    OBJECT_512B,
    OBJECT_1024B,
    OBJECT_2048B,
};

extern struct kmem_cache *kmem_cache; /* kmem_cache for object `struct kmem_cache` allocation. We need this to allocate `struct kmem_cache` */

void slab_init(void);
void free_object(void *object);
void *get_object(unsigned int size);

#endif // __SLAB_H__