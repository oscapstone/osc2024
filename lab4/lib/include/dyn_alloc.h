#ifndef __DYN_ALLOC_H
#define __DYN_ALLOC_H

/**
 * create 8 dynamic pool for size 16, 32, 48, 96, 128, 256, 512, 1024
 * each pool can have maximum 4 pages
 */

#define POOL_NUM 8
#define MAX_PAGE_PER_POOL 4

typedef struct chunk {
    //unsigned int chunk_size;
    struct chunk *next;
} chunk;

typedef struct dynamic_pool {
    unsigned int size_idx;
    unsigned int chunk_num; // chunk num per page
    unsigned int chunk_allocated;
    unsigned int new_chunk_idx;
    unsigned int used_page;
    void* page_list[MAX_PAGE_PER_POOL];
    chunk* free_chunk_list; // chunk being freed will insert to list head
} dynamic_pool;

void dyn_alloc_init();
void *alloc_chunk(unsigned int csize);
void free_chunk(void *addr);

#endif