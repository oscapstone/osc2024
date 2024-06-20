#include "buddy.h"
#include "mini_uart.h"
#include "str_util.h"

static frame_node *frame_list;
static int *frame_status; // -1: free, but it belongs to a larger contiguous memory block, -2: allocated
static int *frame_aord; // list of allocated order
static frame_node* free_list[MAX_ORDER+1] = {0}; // head
static frame_node* free_tail[MAX_ORDER+1] = {0};

extern char __end__;
static char *heap_start_pos = &__end__;
static unsigned long long heap_offset = 0;
// static int frame_num = 0;

void* ini_malloc(unsigned int msize) {
    // allocate "initialized memory"
    char *ptr = (char*)(heap_start_pos + heap_offset);
    heap_offset += msize;
    for(unsigned int i = 0; i < msize; i++) { ptr[i] = 0; }
    return (void*)ptr;
}

/**
 * frame index conversion with real address 0x00000000 - 0x3B400000
 */
unsigned int addr_to_fidx(void* real_addr) {
    unsigned int fidx;
    fidx = ((unsigned long)real_addr - (unsigned long)MEM_START) / FRAME_SIZE;
    return fidx;
}

void* fidx_to_addr(unsigned int fidx) {
    void *real_addr;
    real_addr = (void*)((unsigned long)fidx * FRAME_SIZE + (unsigned long)MEM_START);
    return real_addr;
}

/**
 * frame index conversion with frame_list address
 */
unsigned int get_fidx_from_list(void* list_addr) {
    unsigned int fidx;
    fidx = ((unsigned long)list_addr - (unsigned long)frame_list) / sizeof(frame_node);
    return fidx;
}

void* get_list_from_fidx(unsigned int fidx) {
    void *list_addr;
    list_addr = (void*)((unsigned long)fidx * sizeof(frame_node) + (unsigned long)frame_list);
    return list_addr;
}

unsigned int get_buddy_fidx(unsigned int idx, unsigned int ord) {
    return idx ^ (1 << ord);
}

/**
 * @brief free_list operation: 
 * insert (to free_tail)
 * delete (a specific frame_node)
 * dump (dump free_list head frame index of every order)
 * pop (the first node of free_list[ord])
 */

void free_list_insert(frame_node *new_node, unsigned int ord) {
    if(free_list[ord] == 0) {
        new_node->prev = 0;
        new_node->next = 0;
        free_list[ord] = new_node;
        free_tail[ord] = new_node;
    }
    else {
        new_node->prev = *(free_tail+ord);
        new_node->next = 0;
        free_tail[ord]->next = new_node;
        free_tail[ord] = new_node;
    }
}

void free_list_delete(frame_node* del_node, unsigned int ord) {
    if(del_node->prev != 0) { del_node->prev->next = del_node->next; }
    if(del_node->next != 0) { del_node->next->prev = del_node->prev; }
    if(free_list[ord] == del_node) { free_list[ord] = del_node->next; }
    if(free_tail[ord] == del_node) { free_tail[ord] = del_node->prev; }
}


void free_list_dump() {
    unsigned int idx;
    uart_send_string("[Free list dump first element]\r\n");
    for(int d = MAX_ORDER; d >= 0; d--) {
        uart_send_uint(d);
        if(free_list[d] == 0) { uart_send_string(": empty\r\n"); continue; }
        idx = get_fidx_from_list((void*)free_list[d]);
        uart_send_string(": ");
        uart_send_uint(idx);
        uart_send_string("\r\n");
    }
}

frame_node* free_list_pop(unsigned int ord) {
    frame_node *ptr;
    if(free_list[ord] == 0) { return 0; }
    ptr = free_list[ord];
    if(ptr->next != 0) {
        free_list[ord] = ptr->next;
        free_list[ord]->prev = 0;
    }
    else {
        free_list[ord] = 0;
        free_tail[ord] = 0; 
    }
    return ptr;
}

void frame_status_dump() {
    uart_send_string("frame status dump\r\n");
    for(unsigned int d = 0; d < FRAME_NUM; d++) {
        uart_send_uint(frame_status[d]);
        uart_send(' ');
    }
    uart_send_string("\r\n");
}

void buddy_init() {
    frame_list = ini_malloc(sizeof(frame_node) * FRAME_NUM);
    frame_status = ini_malloc(sizeof(int) * FRAME_NUM);
    frame_aord = ini_malloc(sizeof(int) * FRAME_NUM);

    memset(frame_status, FRAME_ISBUDDY, FRAME_NUM);
    memset(frame_aord, -1, FRAME_NUM);
    for(int i = 0; i < FRAME_NUM; i += BUDDY_ARR_LEN) {
        frame_status[i] = MAX_ORDER;
        free_list_insert(&frame_list[i], MAX_ORDER);
    }
    uart_send_string("buddy_init done\r\n");
    // free_list_dump();
}

void* alloc_frame(unsigned int frame_num) {
    unsigned int alloc_ord = 8 * sizeof(int) - __builtin_clz(frame_num) - 1; 
    // alloc_ord = log2(frame_num), alloc size = 4096 * (2 ^ alloc_ord)
    unsigned int assign_ord, alloc_idx, buddy_idx;
    void *ret = (void*)0;
    frame_node* tmp;

    if((1<<alloc_ord) < frame_num) { alloc_ord++; } // pad to exponents of 2

    for(assign_ord = alloc_ord; assign_ord <= MAX_ORDER; assign_ord++) {
        tmp = free_list_pop(assign_ord);
        if(tmp != 0) {
            alloc_idx = get_fidx_from_list((void*)tmp);
            ret = fidx_to_addr(alloc_idx);

            uart_send_string("[Free block found]: address = 0x");
            uart_send_hex((unsigned long)ret);
            uart_send_string(", order = ");
            uart_send_uint(assign_ord);
            uart_send_string(", idx = ");
            uart_send_uint(alloc_idx);
            uart_send_string("\r\n");

            break;
        }
    }
    //free_list_dump();
    if(tmp == 0) {
        // means that assign_ord > MAX_ORDER
        uart_send_string("[ERROR] Not enough memory\r\n");
        return (void*)0;
    }

    while(assign_ord > alloc_ord) {
        assign_ord--;
        buddy_idx = get_buddy_fidx(alloc_idx, assign_ord);
        frame_status[buddy_idx] = assign_ord;
        free_list_insert(&frame_list[buddy_idx], assign_ord);

        uart_send_string("[Release redundant]: order = ");
        uart_send_uint(assign_ord);
        uart_send_string(", idx = ");
        uart_send_uint(buddy_idx);
        uart_send_string("\r\n");
    }
    
    for(int s = 0; s < (1<<assign_ord); s++) { frame_status[alloc_idx+s] = FRAME_ALLOCATED; }
    frame_aord[alloc_idx] = alloc_ord;
    uart_send_string("[Allocate]: address = 0x");
    uart_send_hex((unsigned long)ret);
    uart_send_string(", order = ");
    uart_send_uint(alloc_ord);
    uart_send_string(", idx = ");
    uart_send_uint(alloc_idx);
    uart_send_string("\r\n");
    
    //free_list_dump();
    return ret;
}

void free_frame(void* addr) {
    unsigned int fidx = addr_to_fidx(addr);
    unsigned int tidx, bidx;
    frame_node *tmp, *bptr;
    
    if(fidx >= FRAME_NUM) { uart_send_string("[ERROR] Not able to free this block\r\n"); return; }
    if(frame_aord[fidx] == -1) { uart_send_string("[ERROR] This block is not allocated\r\n"); return; }
    uart_send_string("[Free]: address = 0x");
    uart_send_hex((unsigned long)addr);
    uart_send_string(", order = ");
    uart_send_uint(frame_aord[fidx]);
    uart_send_string(", idx = ");
    uart_send_uint(fidx);
    uart_send_string("\r\n");
    tmp = (frame_node*)addr;
    tidx = fidx;
    for(int b = frame_aord[fidx]; b <= MAX_ORDER; b++) {
        bidx = get_buddy_fidx(tidx, b);
        if(frame_status[bidx] == b) {
            uart_send_string("[Merge buddy]: order = ");
            uart_send_uint(b);
            uart_send_string(", idx = ");
            uart_send_uint(bidx);
            uart_send_string("\r\n");

            bptr = (frame_node*)get_list_from_fidx(bidx);
            free_list_delete(bptr, b);
            if(bidx < tidx) { tidx = bidx; }
            if(b == MAX_ORDER) {
                tmp = (frame_node*)get_list_from_fidx(tidx);
                free_list_insert(tmp, b);
                for(int f = 0; f < (1<<b); f++) { frame_status[f+tidx] = -1; }
                frame_status[tidx] = b;

                uart_send_string("Free list insert order ");
                uart_send_uint(b);
                uart_send_string("\r\n");
                break;
            }
        }
        else {
            tmp = (frame_node*)get_list_from_fidx(tidx);
            free_list_insert(tmp, b);
            for(int f = 0; f < (1<<b); f++) { frame_status[f+tidx] = -1; }
            frame_status[tidx] = b;

            uart_send_string("Free list insert order ");
            uart_send_uint(b);
            uart_send_string("\r\n");
            break;
        }
    }
}

/**
 * memory reservation
 * reserve_fidx_range, inclusive of end_fidx
 */
void reserve_frame_ord(unsigned int fidx, unsigned int order) {
    unsigned int target_fidx = fidx, buddy_idx;
    unsigned int target_order = order;

    /**
     * fidx is already 2^n
     */
    while (frame_status[target_fidx] < 0) {
            target_fidx &= ~(1 << target_order);
            target_order++;
    }
    target_order = frame_status[target_fidx];
    free_list_delete(&frame_list[target_fidx], frame_status[target_fidx]);

    // uart_send_string("reserve_frame_ord found, idx = ");
    // uart_send_uint(target_fidx);
    // uart_send_string(", order = ");
    // uart_send_uint(target_order);
    // uart_send_string("\r\n");

    while(target_order > order) {
        target_order--;
        buddy_idx = get_buddy_fidx(target_fidx, target_order);
        // uart_send_string("reserve_frame_ord release, idx = ");
        if(target_fidx == fidx) { 
            // request frame is at left side, release right side back to free
            frame_status[buddy_idx] = target_order;
            free_list_insert(&frame_list[buddy_idx], target_order);
            // uart_send_uint(buddy_idx);
        }
        else { 
            // request frame is at right side
            frame_status[target_fidx] = target_order;
            free_list_insert(&frame_list[target_fidx], target_order);
            // uart_send_uint(target_fidx);
        }
        // uart_send_string(", order = ");
        // uart_send_uint(target_order);
        // uart_send_string("\r\n");
    }

    for(int s = 0; s < (1<<target_order); s++) { frame_status[target_fidx+s] = FRAME_RESERVED; }
}

void reserve_fidx_range(unsigned int start_fidx, unsigned int end_fidx, unsigned int order) {
    // uart_send_string("memory reserve frame index range: ");
    // uart_send_uint(start_fidx);
    // uart_send_string(" - ");
    // uart_send_uint(end_fidx);
    // uart_send_string(", current order = ");
    // uart_send_uint(order);
    // uart_send_string("\r\n");

    /**
     * Break down if requested size > size of MAX_ORDER blocks
     */
    if ((order == MAX_ORDER) && ((end_fidx - start_fidx + 1) >= BUDDY_ARR_LEN)) {
            unsigned int head_fidx = start_fidx;
            /**
             * Allocates the start_fidx to idx before next MAX_ORDER block
             */
            if (head_fidx % BUDDY_ARR_LEN != 0) {
                    head_fidx >>= order;
                    head_fidx += 1;
                    head_fidx <<= order;
                    reserve_fidx_range(start_fidx, head_fidx - 1, order - 1);
            }
            /**
             * Allocates the whole block of MAX_ORDER
             */
            while (head_fidx + BUDDY_ARR_LEN <= (end_fidx + 1)) {
                    reserve_frame_ord(head_fidx, order);
                    head_fidx += BUDDY_ARR_LEN;
            }
            /**
             * Allocates the remaining part
             */
            if (head_fidx <= end_fidx) {
                reserve_fidx_range(head_fidx, end_fidx, order - 1);
            }
            return;
    }
    /**
     * Allocates if requested range fits the size of current order
     */
    if (start_fidx % (1 << order) == 0 && (end_fidx + 1) % (1 << order) == 0) {
            reserve_frame_ord(start_fidx, order);
            return;
    }
    /**
     * Allocates block of a smaller order
     */
    order -= 1;
    unsigned int second_id = start_fidx;
    second_id >>= order;
    second_id += 1;
    second_id <<= order;
    if (start_fidx >= second_id || end_fidx < second_id) {
            reserve_fidx_range(start_fidx, end_fidx, order);
            return;
    }
    /**
     * Breaks down if it lays across to blocks of current order
     */
    reserve_fidx_range(start_fidx, second_id - 1, order);
    reserve_fidx_range(second_id, end_fidx, order);
}

void dump_reservation() {
    unsigned int res_flag = 0;
    for(int i = 0; i < FRAME_NUM; i++) {
        if(res_flag == 0 && frame_status[i] == FRAME_RESERVED) {
            res_flag = 1;
            uart_send_uint(i);
            uart_send(' ');
        }
        else if(res_flag == 1 && frame_status[i] != FRAME_RESERVED) {
            res_flag = 0;
            uart_send_uint(i-1);
            uart_send_string("\r\n");
        }
    }
}