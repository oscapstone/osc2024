#include <lib/stdlib.h>

/* kmalloc級別大小配置 */
#define KMALLOC_MIN_ORDER 3
#define KMALLOC_MAX_ORDER 9

/* kmalloc緩存 */
static char kmalloc_caches[KMALLOC_MAX_ORDER - KMALLOC_MIN_ORDER + 1][1 << KMALLOC_MAX_ORDER];

/* 分配記憶體 */
void *kmalloc(size_t size)
{
    unsigned int order = KMALLOC_MIN_ORDER;
    while (size > (1U << order))
        order++;

    char *ptr = kmalloc_caches[order - KMALLOC_MIN_ORDER];
    if (ptr <= kmalloc_caches[order - KMALLOC_MIN_ORDER] + (1U << order)) {
        /* 有可用緩存 */
        kmalloc_caches[order - KMALLOC_MIN_ORDER] = ptr + (1U << order);
        return ptr;
    }

    /* 緩存為空，需要從buddy system獲取新塊 */
    phys_addr_t block = __page_alloc(order);
    if (!block)
        return NULL; /* 記憶體不足 */

    ptr = (char *)block;
    for (unsigned int i = 0; i <= KMALLOC_MAX_ORDER - order; i++) {
        kmalloc_caches[order - KMALLOC_MIN_ORDER] = ptr + (1U << order);
        ptr += (1U << order);
    }
    kmalloc_caches[order - KMALLOC_MIN_ORDER] = (char *)block;

    return (char *)block;
}

/* 釋放記憶體 */
void kfree(void *ptr)
{
    unsigned int order;
    for (order = KMALLOC_MIN_ORDER; order <= KMALLOC_MAX_ORDER; order++) {
        phys_addr_t base = (phys_addr_t)kmalloc_caches[order - KMALLOC_MIN_ORDER];
        phys_addr_t end = base + (1U << order);
        if (base <= (phys_addr_t)ptr && (phys_addr_t)ptr < end) {
            break;
        }
    }

    if (order > KMALLOC_MAX_ORDER) {
        /* 無效指針 */
        return;
    }

    /* 更新kmalloc緩存 */
    char *cache_ptr = kmalloc_caches[order - KMALLOC_MIN_ORDER];
    if (cache_ptr > ptr) {
        kmalloc_caches[order - KMALLOC_MIN_ORDER] = (char *)ptr;
    }

    /* 釋放記憶體塊 */
    __page_free((phys_addr_t)ptr);
}
