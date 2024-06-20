#include "dyn_alloc.h"
#include "buddy.h"
#include "mini_uart.h"
#include "str_util.h"

static dynamic_pool pools[POOL_NUM] = {0};
unsigned int alloc_sizes[POOL_NUM] = {16, 32, 48, 96, 128, 256, 512, 1024};

void pool_init(dynamic_pool *pptr, unsigned int size_idx) {
    pptr->size_idx = size_idx;
    pptr->chunk_num = FRAME_SIZE / alloc_sizes[size_idx]; // chunk num per page
    pptr->chunk_allocated = 0;
    pptr->new_chunk_idx = 0;
    pptr->used_page = 0;
    pptr->free_chunk_list = 0;
}

void dyn_alloc_init() {
    for(int i = 0; i < POOL_NUM; i++) { pool_init(&pools[i], i); }
}

void *alloc_chunk(unsigned int csize) {
    unsigned int aidx;
    for(aidx = 0; aidx < POOL_NUM; aidx++) {
        if(alloc_sizes[aidx] >= csize) { break; }
    }
    /**
     * first check if there is free chunk
     * then check if pool reach maximum
     * then check if pool needs to fetch a new page
     * finally true alloc
     */
    if(pools[aidx].free_chunk_list != 0) {
        void *ret = (void*) pools[aidx].free_chunk_list;
        pools[aidx].free_chunk_list = pools[aidx].free_chunk_list->next;
        uart_send_string("[Free chunk found]: address = 0x");
        uart_send_hex((unsigned long)ret);
        uart_send_string(", in pool size ");
        uart_send_uint(alloc_sizes[aidx]);
        uart_send_string("\r\n");
        return ret;
    }

    if(pools[aidx].chunk_allocated >= pools[aidx].chunk_num * MAX_PAGE_PER_POOL) {
        uart_send_string("[ERROR] pool size ");
        uart_send_uint(alloc_sizes[aidx]);
        uart_send_string(" maximum reached\r\n");
        return (void*)0;
    }

    if(pools[aidx].chunk_allocated >= pools[aidx].chunk_num * pools[aidx].used_page) {
        pools[aidx].page_list[pools[aidx].used_page] = alloc_frame(1);
        uart_send_string("[DYNALLOC] pool size ");
        uart_send_uint(alloc_sizes[aidx]);
        uart_send_string(" fetch a new page, address = 0x");
        uart_send_hex((unsigned long)pools[aidx].page_list[pools[aidx].used_page]);
        uart_send_string("\r\n");
        pools[aidx].used_page++;
        pools[aidx].new_chunk_idx = 0;
    }

    void *ret = pools[aidx].page_list[pools[aidx].used_page-1] + (alloc_sizes[aidx] * pools[aidx].new_chunk_idx);
    pools[aidx].new_chunk_idx ++;
    pools[aidx].chunk_allocated ++;
    uart_send_string("[Allocate]: address = 0x");
    uart_send_hex((unsigned long)ret);
    uart_send_string(", pool size ");
    uart_send_uint(alloc_sizes[aidx]);
    uart_send_string("\r\n");
    return ret;
}

void free_chunk(void *addr) {
    // since frame size = 0x1000, bitmask = ~0xfff
    void *page_base = (void*)((unsigned long)addr & (~0xfff));
    unsigned int pidx = 0;
    
    for(int i = 0; i < POOL_NUM; i++) {
        for(int j = 0; j < pools[i].used_page; j++) {
            if((unsigned long)page_base == (unsigned long)pools[i].page_list[j]) {
                pidx = i;
                break;
            }
        }
    }

    if(pidx == 0) {
        uart_send_string("[ERROR] chunk don't belong to any page in pool\r\n");
        return;
    }

    pools[pidx].chunk_allocated--;
    chunk *ptr = (chunk*)addr;
    ptr->next = pools[pidx].free_chunk_list;
    pools[pidx].free_chunk_list = ptr;
    uart_send_string("[DYNALLOC] chunk free to pool size ");
    uart_send_uint(alloc_sizes[pidx]);
    uart_send_string("\r\n");
}
