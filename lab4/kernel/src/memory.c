#include "memory.h"
#include "u_list.h"
#include "uart1.h"
#include "exception.h"
#include "dtb.h"

extern char  _heap_top;
static char* htop_ptr = &_heap_top;

extern char  _start;
extern char  _end;
extern char  _stack_top;
extern char* CPIO_DEFAULT_START;
extern char* CPIO_DEFAULT_END;
extern char* dtb_ptr;

// ------ Lab2 ------
void* s_allocator(unsigned int size)
{
    // -> htop_ptr
    // htop_ptr + 0x02:  heap_block size
    // htop_ptr + 0x10 ~ htop_ptr + 0x10 * k:
    //            { heap_block }
    // -> htop_ptr

    // 0x10 for heap_block header
    char* r = htop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned int*)(r - 0x8) = size;
    htop_ptr += size;
    return r;
}

void s_free(void* ptr)
{
    // TBD
}

// ------ Lab4 ------
static frame_t* frame_array;                    // store memory's statement and page's corresponding index
static list_head_t        frame_freelist[FRAME_MAX_IDX];  // store available block for page
static list_head_t        chunk_list[CHUNK_MAX_IDX];      // store available block for chunk

void init_allocator()
{   //allocator in heap, simple allocator
    frame_array = s_allocator(BUDDY_MEMORY_PAGE_COUNT * sizeof(frame_t));

    // init frame_array
    for (int i = 0; i < BUDDY_MEMORY_PAGE_COUNT; i++)
    {
        if (i % (1 << FRAME_IDX_FINAL) == 0)
        {//val : order
            frame_array[i].val = FRAME_IDX_FINAL;
            frame_array[i].used = FRAME_FREE;
        }
    }

    //init frame freelist
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
    {
        INIT_LIST_HEAD(&frame_freelist[i]);
    }

    //init chunk list
    for (int i = CHUNK_IDX_0; i <= CHUNK_IDX_FINAL; i++)
    {
        INIT_LIST_HEAD(&chunk_list[i]);
    }

    for (int i = 0; i < BUDDY_MEMORY_PAGE_COUNT; i++)
    {
        // init listhead for each frame
        INIT_LIST_HEAD(&frame_array[i].listhead);
        frame_array[i].idx = i;
        frame_array[i].chunk_order = CHUNK_NONE;

        // add init frame (FRAME_IDX_FINAL) into freelist
        if (i % (1 << FRAME_IDX_FINAL) == 0)
        {
            list_add(&frame_array[i].listhead, &frame_freelist[FRAME_IDX_FINAL]);
        }
    }

    /* Startup reserving the following region:
    Spin tables for multicore boot (0x0000 - 0x1000)
    Devicetree (Optional, if you have implement it)
    Kernel image in the physical memory
    Your simple allocator (startup allocator) (Stack + Heap in my case)
    Initramfs
    */
    uart_sendline("\r\n* Startup Allocation *\r\n");
    uart_sendline("buddy system: usable memory region: 0x%x ~ 0x%x\n", BUDDY_MEMORY_BASE, BUDDY_MEMORY_BASE + BUDDY_MEMORY_PAGE_COUNT * PAGESIZE);
    dtb_find_and_store_reserved_memory(); // find spin tables in dtb
    memory_reserve((unsigned long long) & _start, (unsigned long long) & _end); // kernel
    memory_reserve((unsigned long long) & _heap_top, (unsigned long long) & _stack_top);  // heap & stack -> simple allocator
    memory_reserve((unsigned long long)CPIO_DEFAULT_START, (unsigned long long)CPIO_DEFAULT_END);
}

// void print_allocated_pages_addr() {
//     uart_sendline("Currently allocated addresses:");
//     int allocated_count = 0;

//     for (int i = 0; i < BUDDY_MEMORY_PAGE_COUNT; i++) {
//         if (frame_array[i].used == FRAME_ALLOCATED) {
//             unsigned long long address = BUDDY_MEMORY_BASE + (i * PAGESIZE);
//             uart_sendline(" 0x%x", address);
//             allocated_count++;
//         }
//     }

//     if (allocated_count == 0) {
//         uart_sendline(" No allocated addresses currently.\n");
//     } else {
//         uart_sendline("\nTotal allocated addresses: %d.\n", allocated_count);
//     }

//     uart_sendline("----------------------------\n");
// }

// struct frame_t *list_entry(struct list_head *list) {
//     // 根據你的情況調整偏移量，以獲取 frame_t 結構
//     return (struct frame_t *)((char *)list - offsetof(struct frame_t, list));
// }


void print_allocated_pages_addr()
{
    // struct list_head *tmp;
    // uart_sendline("\n------- BS free list -------\n");
    // for (int i = 0; i < FRAME_MAX_IDX; i++) {
    //     uart_sendline("Order ");
    //     if (i < 10) uart_sendline(" ");
    //     uart_sendline("%d free list index: ",i);
    //     list_for_each(tmp, &frame_freelist[i])
    //     {
    //         // uart_2hex(list_entry(tmp, struct frame_t, list)->idx);
    //         struct frame_t *frame = get_frame_from_list(tmp);
    //         uart_2hex(frame->idx);
    //         uart_sendline(" ");
    //     }
    //     uart_sendline("\n");
    // }
    // uart_sendline("\n----------------------------\n");

    // return;
}

void print_allocated_chunks_addr() {
    uart_sendline("Currently allocated chunk addresses:");
    int allocated_count = 0;

    // 遍歷所有的 chunk 清單，找出被分配的 chunk 地址
    for (int i = CHUNK_IDX_0; i <= CHUNK_IDX_FINAL; i++) {
        int chunk_size = (32 << i);  // 取得每個 chunk 大小
        list_head_t *current = chunk_list[i].next;

        // 列印每個未在空閒清單中的 chunk 地址
        while (current != &chunk_list[i]) {
            char *chunk_address = (char *)current;
            frame_t *page_frame = &frame_array[((unsigned long long)chunk_address - BUDDY_MEMORY_BASE) >> 12];

            // 檢查該 chunk 是否被分配：不在當前 chunk 清單即為分配
            if (page_frame->chunk_order != i) {
                uart_sendline(" 0x%x (size: %dB)", chunk_address, chunk_size);
                allocated_count++;
            }

            current = current->next;
        }
    }

    if (allocated_count == 0) {
        uart_sendline(" No allocated chunks currently.\n");
    } else {
        uart_sendline("\nTotal allocated chunks: %d.\n", allocated_count);
    }

    uart_sendline("----------------------------\n");
}


void* page_malloc(unsigned int size)
{
    uart_sendline("    [+] Allocate page - size : %d(0x%x)\r\n", size, size);
    uart_sendline("        Before\r\n");
    dump_page_info();

    int target_order;
    // turn size into minimum 4KB * 2**target_order
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
    {
        // search from small to big
        if (size <= (PAGESIZE << i))
        {
            target_order = i;
            uart_sendline("        block size = 0x%x\n", PAGESIZE << i);
            break;
        }
        if (i == FRAME_IDX_FINAL)
        {
            uart_puts("[!] request size exceeded for page_malloc!!!!\r\n");
            return (void*)0;
        }

    }

    // find the available larger free page in freelist
    int order;
    for (order = target_order; order <= FRAME_IDX_FINAL; order++)
    {
        // freelist does not have 2**i order frame, going for next order
        if (!list_empty(&frame_freelist[order]))
            break;
    }
    if (order > FRAME_IDX_FINAL)
    {
        uart_puts("[!] No available frame in freelist, page_malloc ERROR!!!!\r\n");
        return (void*)0;
    }

    // get the available frame from freelist
    // 找當前order的freelist的第一個frame
    frame_t* target_frame_ptr = (frame_t*)frame_freelist[order].next;
    list_del_entry((struct list_head*)target_frame_ptr);

    // // 計算目標 frame 的基地址
    unsigned long frame_base = BUDDY_MEMORY_BASE + (target_frame_ptr->idx * PAGESIZE);

    // 目前不夠用會去比較大的地方切割
    // Release redundant memory block to separate into pieces
    for (int j = order; j > target_order; j--) // ex: 10000 -> 01111
    {
        // ex 8kb-> 4kb, 4kb
        unsigned long left_block_start = frame_base;
        unsigned long left_block_end = left_block_start + (PAGESIZE << (j - 1)) - 1;
        unsigned long right_block_start = left_block_end + 1;
        unsigned long right_block_end = right_block_start + (PAGESIZE << (j - 1)) - 1;

        //uart_sendline("dump_page_info big to small:\n");
        // dump_page_info();
        // // 輸出分割訊息
        uart_sendline("Split 0x%x to 0x%x ~ 0x%x and 0x%x ~ 0x%x\n",
            (unsigned int)frame_base,
            (unsigned int)left_block_start, (unsigned int)left_block_end,
            (unsigned int)right_block_start, (unsigned int)right_block_end);
        release_redundant(target_frame_ptr);//split
        
    }

    
    // Allocate it
    // return allocated buddy memory address
    target_frame_ptr->used = FRAME_ALLOCATED;
    uart_sendline("        physical address : 0x%x\n", BUDDY_MEMORY_BASE + (PAGESIZE * (target_frame_ptr->idx)));
    uart_sendline("        After\r\n");
    dump_page_info();

    return (void*)BUDDY_MEMORY_BASE + (PAGESIZE * (target_frame_ptr->idx));
}

void page_free(void* ptr)
{   
    //idx : ((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12 
    frame_t* target_frame_ptr = &frame_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12]; // PAGESIZE * Available Region -> 0x1000 * 0x10000000 // SPEC #1, #2
    uart_sendline("    [+] Free page: 0x%x, val = %d\r\n", ptr, target_frame_ptr->val);
    uart_sendline("        Before\r\n");
    dump_page_info();
    target_frame_ptr->used = FRAME_FREE;
    while (coalesce(target_frame_ptr) == 0); // merge buddy iteratively
    list_add(&target_frame_ptr->listhead, &frame_freelist[target_frame_ptr->val]);
    uart_sendline("        After\r\n");
    dump_page_info();
}

frame_t* release_redundant(frame_t* frame)//split
{
    // order -1 -> add its buddy to free list (frame itself will be used in master function)
    // 切割出來的兩塊, 有一塊被aloocated掉沒有加進去

    frame->val -= 1;
    frame_t* buddyptr = get_buddy(frame);
    buddyptr->val = frame->val;
    list_add(&buddyptr->listhead, &frame_freelist[buddyptr->val]);
    //dump_page_info();
    return frame;
}

frame_t* get_buddy(frame_t* frame)
{
    // XOR(idx, order)
    return &frame_array[frame->idx ^ (1 << frame->val)];
}

int coalesce(frame_t* frame_ptr)
{
    frame_t* buddy = get_buddy(frame_ptr);
    // frame is the boundary
    if (frame_ptr->val == FRAME_IDX_FINAL)
        return -1;

    // Order must be the same: 2**i + 2**i = 2**(i+1)
    if (frame_ptr->val != buddy->val)
        return -1;

    // buddy is in used
    if (buddy->used == FRAME_ALLOCATED)
        return -1;

    list_del_entry((struct list_head*)buddy);
    // merge成更大塊，所以更新他的order值
    frame_ptr->val += 1;
    uart_sendline("    coalesce detected, merging 0x%x, 0x%x, -> val = %d\r\n", frame_ptr->idx, buddy->idx, frame_ptr->val);
    return 0;
}

void dump_page_info()
{
    unsigned int exp2 = 1;
    uart_sendline("        ----------------- [  Number of Available Page Blocks  ] -----------------\r\n        | ");
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
    {
        uart_sendline("%4dKB(%1d) ", 4 * exp2, i);
        exp2 *= 2;
    }
    uart_sendline("|\r\n        | ");
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
        uart_sendline("     %4d ", list_size(&frame_freelist[i]));
    uart_sendline("|\r\n");
}

void dump_chunk_info()
{
    unsigned int exp2 = 1;
    uart_sendline("    -- [  Number of Available Chunk Blocks ] --\r\n    | ");
    for (int i = CHUNK_IDX_0; i <= CHUNK_IDX_FINAL; i++)
    {
        uart_sendline("%4dB(%1d) ", 32 * exp2, i);
        exp2 *= 2;
    }
    uart_sendline("|\r\n    | ");
    for (int i = CHUNK_IDX_0; i <= CHUNK_IDX_FINAL; i++)
        uart_sendline("   %5d ", list_size(&chunk_list[i]));
    uart_sendline("|\r\n");
}

void page2chunks(int target_order)
{
    // make chunks from a smallest-size page
    char* page = page_malloc(PAGESIZE);
    frame_t* pageframe_ptr = &frame_array[((unsigned long long)page - BUDDY_MEMORY_BASE) >> 12];
    pageframe_ptr->chunk_order = target_order;

    // split page into a lot of chunks and push them into chunk_list
    int chunksize = (32 << target_order);
    for (int i = 0; i < PAGESIZE; i += chunksize)
    {
        list_head_t* c = (list_head_t*)(page + i); //page = chunk base address, i is i-th chunk
        list_add(c, &chunk_list[target_order]); // c : chunk addr
    }
}

void* chunk_malloc(unsigned int size)
{
    uart_sendline("[+] Allocate chunk - size : %d(0x%x)\r\n", size, size);
    uart_sendline("    Before\r\n");
    dump_chunk_info();

    // turn size into chunk order: 32B * 2**target_order
    int target_order;
    for (int i = CHUNK_IDX_0; i <= CHUNK_IDX_FINAL; i++)
    {
        if (size <= (32 << i)) { target_order = i; break; }
    }

    // if no available chunk in list, assign one page for it
    if (list_empty(&chunk_list[target_order]))
    {
        page2chunks(target_order);
    }

    list_head_t* r = chunk_list[target_order].next;
    list_del_entry(r);
    uart_sendline("    physical address : 0x%x\n", r);
    uart_sendline("    After\r\n");
    dump_chunk_info();
    return r;
}

int all_chunks_free_in_page(frame_t* page_frame)
{
    // 獲取該頁面的 chunk 級別
    int chunk_order = page_frame->chunk_order;
    int chunksize = (32 << chunk_order);
    int num_chunks_in_page = PAGESIZE / chunksize;

    // 計算該頁面起始地址
    unsigned long long page_start = BUDDY_MEMORY_BASE + (page_frame->idx * PAGESIZE);
    unsigned long long page_end = page_start + PAGESIZE;

    // 檢查該頁面內的所有 chunk
    int free_chunks_count = 0;
    list_head_t *current;
    list_for_each(current, &chunk_list[chunk_order]) {
        if ((unsigned long long)current >= page_start && (unsigned long long)current < page_end) {
            free_chunks_count++;
        }
    }

    // 如果該頁面內所有 chunk 都已釋放，則返回 true
    return (free_chunks_count == num_chunks_in_page);
}

void chunk_free(void* ptr)
{
    list_head_t* c = (list_head_t*)ptr;
    frame_t* pageframe_ptr = &frame_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12];
    uart_sendline("[+] Free chunk: 0x%x, val = %d\r\n", ptr, pageframe_ptr->chunk_order);
    uart_sendline("    Before\r\n");
    dump_chunk_info();
    list_add(c, &chunk_list[pageframe_ptr->chunk_order]);
    uart_sendline("    After\r\n");
    dump_chunk_info();
    // uart_sendline("%d", 32 * pageframe_ptr->chunk_order << 1 * list_size(&chunk_list[pageframe_ptr->chunk_order]));
    // if (32 * 1<<pageframe_ptr->chunk_order * list_size(&chunk_list[pageframe_ptr->chunk_order]) == PAGESIZE)
    // {
    //     page_free(ptr);
    // }
    if (all_chunks_free_in_page(pageframe_ptr)) {
        uart_sendline("[+] All chunks in page are free. Releasing page back to Buddy System\n");
        unsigned long long address = BUDDY_MEMORY_BASE + (pageframe_ptr->idx * PAGESIZE);
        page_free((void*)address);
    }

}

void* kmalloc(unsigned int size)
{
    uart_sendline("\n\n");
    uart_sendline("================================\r\n");
    uart_sendline("[+] Request kmalloc size: %d\r\n", size);
    uart_sendline("================================\r\n");
    // if size is larger than chunk size, go for page
    if (size > (32 << CHUNK_IDX_FINAL)) // if > 2048 bytes
    {
        void* r = page_malloc(size);
        return r;
    }
    // go for chunk
    void* r = chunk_malloc(size);
    return r;
}

void kfree(void* ptr)
{
    uart_sendline("\n\n");
    uart_sendline("==========================\r\n");
    uart_sendline("[+] Request kfree 0x%x\r\n", ptr);
    uart_sendline("==========================\r\n");
    // If no chunk assigned, go for page
    if ((unsigned long long)ptr % PAGESIZE == 0 && frame_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12].chunk_order == CHUNK_NONE)
    {
        page_free(ptr);
        return;
    }
    // go for chunk
    chunk_free(ptr);
}

void memory_reserve(unsigned long long start, unsigned long long end)
{
    start -= start % PAGESIZE; // floor (align 0x1000)
    end = end % PAGESIZE ? end + PAGESIZE - (end % PAGESIZE) : end; // ceiling (align 0x1000)

    uart_sendline("Reserved Memory: ");
    uart_sendline("start 0x%x ~ ", start);
    uart_sendline("end 0x%x\r\n", end);

    // delete page from free list
    for (int order = FRAME_IDX_FINAL; order >= 0; order--)
    {
        list_head_t* pos;
        list_for_each(pos, &frame_freelist[order])
        {
            unsigned long long pagestart = ((frame_t*)pos)->idx * PAGESIZE + BUDDY_MEMORY_BASE;
            unsigned long long pageend = pagestart + (PAGESIZE << order);

            if (start <= pagestart && end >= pageend) // if page all in reserved memory -> delete it from freelist
            {
                ((frame_t*)pos)->used = FRAME_ALLOCATED;
                uart_sendline("    [!] Reserved page in 0x%x - 0x%x\n", pagestart, pageend);
                // uart_sendline("        Before\n");
                // dump_page_info();
                // list_del_entry(pos);
                // uart_sendline("        Remove usable block for reserved memory: order %d\r\n", order);
                // uart_sendline("        After\n");
                // dump_page_info();
            } else if (start >= pageend || end <= pagestart) // no intersection
            {
                continue;
            } else // partial intersection, separate the page into smaller size.
            {
                list_del_entry(pos);
                list_head_t* temppos = pos->prev;
                list_add(&release_redundant((frame_t*)pos)->listhead, &frame_freelist[order - 1]);// recursion reduant memory
                pos = temppos;
            }
        }
    }
}
