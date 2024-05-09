#include "memory.h"
#include "u_list.h"
#include "uart1.h"
#include "exception.h"
#include "dtb.h"
#include "shell.h"

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

void startup_allocation()
{
    char input_buffer[CMD_MAX_LEN];
    /* Startup reserving the following region:
    Spin tables for multicore boot (0x0000 - 0x1000)
    Devicetree (Optional, if you have implement it)
    Kernel image in the physical memory
    Your simple allocator (startup allocator) (Stack + Heap in my case)
    Initramfs
    */
    //0~983040*4=3932160 kb, 3C000 pages
    uart_sendline("\r\n* Startup Allocation *\r\n");
    uart_sendline("buddy system: usable memory region: 0x%x ~ 0x%x\n", BUDDY_MEMORY_BASE, BUDDY_MEMORY_BASE + BUDDY_MEMORY_PAGE_COUNT * PAGESIZE);

    //4 kb
    uart_sendline("\r\n* dtb_find_and_store *\r\n");
    dtb_find_and_store_reserved_memory(); // find spin tables in dtb
    cli_cmd_read(input_buffer);

    //41 kb
    uart_sendline("\r\n* Reserve kernel: 0x%x ~ 0x%x *\r\n", &_start, &_end);
    memory_reserve((unsigned long long) & _start, (unsigned long long) & _end); // kernel
    cli_cmd_read(input_buffer);

    //8096 kb
    //uart_sendline("\r\n* Reserve heap & stack: 0x%x ~ 0x%x *\r\n", &_heap_top, &_stack_top);
    //memory_reserve((unsigned long long) & _heap_top, (unsigned long long) & _stack_top);  // heap & stack -> simple allocator

    //1 kb
    uart_sendline("\r\n* Reserve CPIO: 0x%x ~ 0x%x *\r\n", CPIO_DEFAULT_START, CPIO_DEFAULT_END);
    memory_reserve((unsigned long long)CPIO_DEFAULT_START, (unsigned long long)CPIO_DEFAULT_END);
    cli_cmd_read(input_buffer);
}

void init_allocator()
{   //allocator in heap, simple allocator
    frame_array = s_allocator(BUDDY_MEMORY_PAGE_COUNT * sizeof(frame_t)); //48*0x3C000=774144 bytes=756kb
    // init frame_array
    for (int i = 0; i < BUDDY_MEMORY_PAGE_COUNT; i++)
    { 
        frame_array[i].val = FRAME_IDX_0;
        frame_array[i].used = FRAME_FREE;
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
        list_add_tail(&frame_array[i].listhead, &frame_freelist[FRAME_IDX_0]);   
    }

    startup_allocation();

    for(int level = 0; level < FRAME_MAX_IDX; level++)
    {
      for (int i = 0; i < BUDDY_MEMORY_PAGE_COUNT; i+=(1<<level))
      {
        merge_block1(&frame_array[i]);
      }
    }




    print_allocated_pages_addr();
    print_allocated_chunks_addr();
    //print_allocated_pages_index();
    uart_sendline("Buddy System Init Done\nHeap top: 0x");
    uart_2hex((unsigned int)&_heap_top);
    uart_sendline("\nFrame Freelist Last Address: 0x");
    uart_2hex((unsigned int)&frame_freelist[FRAME_MAX_IDX] + 48);
    uart_sendline("\nFrame Freelist Last Address: 0x");
    uart_2hex((unsigned int)&chunk_list[CHUNK_MAX_IDX] + 48);
}

void print_allocated_pages_addr()
{
    struct list_head *tmp;
    uart_sendline("\n------- BS free list -------\n");
    for (int i = 0; i < FRAME_MAX_IDX; i++) {
        uart_sendline("Order ");
        if (i < 10) uart_sendline(" ");
        uart_sendline("%d free list address: ",i);
        list_for_each(tmp, &frame_freelist[i])
        {
            // uart_2hex(list_entry(tmp, struct frame_t, list)->idx);
            struct frame* frame = tmp;//get_frame_from_list(tmp);
            uart_2hex(frame->idx<<12);
            //uart_sendline("%x", frame->idx);
            uart_sendline(" ");
        }
        uart_sendline("\n");
    }
    uart_sendline("\n----------------------------\n");

    //print_allocated_pages_index();
    return;
}

void print_allocated_pages_index() {
    uart_sendline("Currently allocated address:");
    int allocated_count = 0;

    for (int i = 0; i < MAX_PAGES; i++) {
        if (frame_array[i].used == FRAME_ALLOCATED) {
            uart_sendline(" %x ",i<<12);
            //uart_2hex(i);
            //uart_sendline(" ");
            allocated_count++;
        }
    }

    if (allocated_count == 0) {
        uart_sendline(" No allocations currently.\n");
    } else {
        uart_sendline("\nTotal allocated blocks: %d blocks.\n",allocated_count);
    }
    
    uart_sendline("----------------------------\n");
}

void print_allocated_chunks_addr() {
    uart_sendline("\n------- Chunk List -------\n");
    for (int i = CHUNK_IDX_0; i <= CHUNK_IDX_FINAL; i++) {
        uart_sendline("Order %d (size: %d bytes) address: ", i, 32 << i);
        struct list_head *tmp;
        
        // 遍歷指定 order 的 chunk 列表
        list_for_each(tmp, &chunk_list[i]) {
            struct list_head *chunk = tmp;
            // 計算 chunk 的起始地址
            unsigned long long chunk_address = (unsigned long long)chunk;
            
            uart_2hex(chunk_address);
            uart_sendline(" ");
        }
        uart_sendline("\n");
    }
    uart_sendline("\n-------------------------\n");
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

    // find the smallest larger frame in freelist
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

    // 計算目標 frame 的 base address
    unsigned long frame_base = BUDDY_MEMORY_BASE + (target_frame_ptr->idx * PAGESIZE);

    //如果在freelist都找不到，就要split block
    // Release redundant memory block to separate into pieces
    for (int j = order; j > target_order; j--) // ex: 10000 -> 01111
    {
        split_block(target_frame_ptr);
    }

    
    // Allocate it
    target_frame_ptr->used = FRAME_ALLOCATED;
    uart_sendline("        physical address : 0x%x\n", BUDDY_MEMORY_BASE + (PAGESIZE * (target_frame_ptr->idx)));
    uart_sendline("        After\r\n");
    dump_page_info();

    return (void*)BUDDY_MEMORY_BASE + (PAGESIZE * (target_frame_ptr->idx));
}

void page_free(void* ptr)
{
    frame_t* target_frame_ptr = &frame_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12]; // PAGESIZE * Available Region -> 0x1000 * 0x10000000 // SPEC #1, #2
    uart_sendline("    [+] Free page: 0x%x, val = %d\r\n", ptr, target_frame_ptr->val);
    uart_sendline("        Before\r\n");
    dump_page_info();
    target_frame_ptr->used = FRAME_FREE;
    while (merge_block(target_frame_ptr) == 0); // merge buddy iteratively
    list_add(&target_frame_ptr->listhead, &frame_freelist[target_frame_ptr->val]);
    uart_sendline("        After\r\n");
    dump_page_info();
}

frame_t* split_block(frame_t* frame)
{
    // order -1 -> add its buddy to free list (frame itself will be used in master function)
    frame->val -= 1;

    // Calculate the starting addresses for the left and right blocks based on the current frame.
    unsigned long left_block_start = frame->idx * PAGESIZE + BUDDY_MEMORY_BASE;
    unsigned long left_block_end = left_block_start + (PAGESIZE << frame->val) - 1;
    unsigned long right_block_start = left_block_end + 1;
    unsigned long right_block_end = right_block_start + (PAGESIZE << frame->val) - 1;

    frame_t* buddyptr = get_buddy(frame);
    buddyptr->val = frame->val;
    list_add(&buddyptr->listhead, &frame_freelist[buddyptr->val]);

    // Output the split information to the UART.
    uart_sendline("Split 0x%x to 0x%x ~ 0x%x and 0x%x ~ 0x%x\n",
        (unsigned int)(frame->idx * PAGESIZE + BUDDY_MEMORY_BASE),
        (unsigned int)left_block_start, (unsigned int)left_block_end,
        (unsigned int)right_block_start, (unsigned int)right_block_end);
    
    dump_page_info();
    return frame;
}

frame_t* get_buddy(frame_t* frame)
{
    // XOR(idx, order)
    return &frame_array[frame->idx ^ (1 << frame->val)];
}
int merge_block(frame_t* frame_ptr)
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
    frame_ptr->val += 1;
  
    uart_sendline("    Merging 0x%x, 0x%x, -> val = %d\r\n", frame_ptr->idx, buddy->idx, frame_ptr->val);
    return 0;
}

int merge_block1(frame_t* frame_ptr)
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
    list_del_entry((struct list_head*)frame_ptr);
    frame_ptr->val += 1;
    list_add((struct list_head*)frame_ptr, &frame_freelist[frame_ptr->val]);
 
    //uart_sendline("    Merging 0x%x, 0x%x, -> val = %d\r\n", frame_ptr->idx, buddy->idx, frame_ptr->val);
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

void page2chunks(int target_chunk_order)
{
    // make chunks from a smallest-size page
    char* page = page_malloc(PAGESIZE);
    frame_t* pageframe_ptr = &frame_array[((unsigned long long)page - BUDDY_MEMORY_BASE) >> 12];
    pageframe_ptr->chunk_order = target_chunk_order;

    // split page into a lot of chunks and push them into chunk_list
    int chunksize = (32 << target_chunk_order);
    for (int i = 0; i < PAGESIZE; i += chunksize)
    {
        list_head_t* c = (list_head_t*)(page + i); //page = chunk base address, i is i-th chunk
        list_add(c, &chunk_list[target_chunk_order]); // c : chunk addr
    }
}

void* chunk_malloc(unsigned int size)
{
    uart_sendline("[+] Allocate chunk - size : %d(0x%x)\r\n", size, size);
    uart_sendline("    Before\r\n");
    dump_chunk_info();

    // turn size into chunk order: 32B * 2**target_order
    int target_chunk_order;
    for (int i = CHUNK_IDX_0; i <= CHUNK_IDX_FINAL; i++)
    {
        if (size <= (32 << i)) { target_chunk_order = i; break; }
    }

    // if no available chunk in list, assign one page for it
    if (list_empty(&chunk_list[target_chunk_order]))
    {
        page2chunks(target_chunk_order);
    }

    list_head_t* r = chunk_list[target_chunk_order].next;
    list_del_entry(r);
    uart_sendline("    physical address : 0x%x\n", r);
    uart_sendline("    After\r\n");
    dump_chunk_info();
    return r;
}

int all_chunks_free_and_return_page(frame_t* page_frame)
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
    if (free_chunks_count == num_chunks_in_page)
    {
        list_for_each(current, &chunk_list[chunk_order])
        {
            if ((unsigned long long)current >= page_start && (unsigned long long)current < page_end) {
                list_del_entry(current);
            }
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
    if (all_chunks_free_and_return_page(pageframe_ptr)) {
        uart_sendline("[+] All chunks in page are free. Releasing page back to Buddy System\n");

        page_free((void*)(BUDDY_MEMORY_BASE + (pageframe_ptr->idx * PAGESIZE)));
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
    void* r = chunk_malloc(size+16); // 16 bytes for (head)metadata
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

void memory_reserve(unsigned long long start, unsigned long long end) {
    // Calculate the range of pages to be reserved
    int start_page = (start - BUDDY_MEMORY_BASE) / PAGESIZE;
    int end_page = (end - BUDDY_MEMORY_BASE - 1) / PAGESIZE;

    // uart_sendline("Reserved Memory: ");
    // uart_sendline("start_page 0x%x ~ ", start_page);
    // uart_sendline("end_page 0x%x\r\n", end_page);
    uart_sendline("    [!] Reserved page in 0x%x - 0x%x\n", start_page, end_page);
    uart_sendline("        Before\n");
    dump_page_info();
    //uart_sendline("        Remove usable block for reserved memory: order %d\r\n", order);

    // Mark all pages within the specified range as allocated
    for (int i = start_page; i <= end_page; i++) {
        frame_array[i].used = FRAME_ALLOCATED;
        list_del_init(&frame_array[i].listhead);
    }
    uart_sendline("        After\n");
    dump_page_info();
}
