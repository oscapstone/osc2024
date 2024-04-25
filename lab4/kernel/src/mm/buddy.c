#include <lib/stdlib.h>

/* Buddy system常數和類型定義 */
#define MAX_ORDER 10
typedef uint32_t *phys_addr_t;

/* 自由鏈表節點 */
typedef struct free_area_node {
    struct free_area_node *next;
    unsigned long order;
} free_area_node_t;

/* 自由鏈表 */
static free_area_node_t *free_areas[MAX_ORDER + 1];

/* 初始化buddy system */
void buddy_init(phys_addr_t start, phys_addr_t end) {
    unsigned long buddy_size = end - start;
    unsigned long max_order = MAX_ORDER - __builtin_clzl(buddy_size);
    free_areas[max_order] = (free_area_node_t *)start;
    free_areas[max_order]->order = max_order;
    for (int i = 0; i < max_order; i++) {
        free_areas[i] = NULL;
    }
}

/* 分配記憶體 */
phys_addr_t __page_alloc(unsigned long order) {
    unsigned long cur_order;
    free_area_node_t *node;

    for (cur_order = order; cur_order <= MAX_ORDER; cur_order++) {
        if (free_areas[cur_order]) {
            node = free_areas[cur_order];
            free_areas[cur_order] = node->next;
            break;
        }
    }

    if (cur_order > MAX_ORDER) return 0; /* 記憶體不足 */

    for (cur_order = order; cur_order < cur_order; cur_order++) {
        phys_addr_t buddy = (phys_addr_t)node + (1UL << cur_order);
        free_areas[cur_order] = (free_area_node_t *)buddy;
        free_areas[cur_order]->order = cur_order;
    }

    return (phys_addr_t)node;
}

/* 釋放記憶體 */
void __page_free(phys_addr_t addr) {
    free_area_node_t *node = (free_area_node_t *)addr;
    unsigned long order = node->order;

    while (order < MAX_ORDER) {
        phys_addr_t buddy = addr ^ (1UL << order);
        if ((free_area_node_t *)buddy >= node) break;
        free_area_node_t *buddy_node = (free_area_node_t *)buddy;
        if (buddy_node->order != order) break;

        /* 合併相鄰記憶體塊 */
        addr = addr < buddy ? addr : buddy;
        node = (free_area_node_t *)addr;
        node->order = order + 1;
        order += 1;

        /* 從鏈表中移除相鄰節點 */
        buddy = buddy > addr ? buddy : addr;
        buddy_node = (free_area_node_t *)buddy;
        free_area_node_t **prev = &free_areas[order];
        while (*prev != buddy_node) prev = &(*prev)->next;
        *prev = buddy_node->next;
    }

    /* 插入已合併的記憶體塊 */
    node->next = free_areas[order];
    free_areas[order] = node;
}
