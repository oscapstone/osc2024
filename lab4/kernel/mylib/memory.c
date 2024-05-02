#include "uint.h"
#include "uart.h"
#include "memory.h"
#include "malloc.h"
#include "dtb.h"

extern char *cpio_start;
extern char *cpio_end;
extern char __text_start;
extern char __heap_start;
extern char __startup_allocator_start;
extern char __startup_allocator_end;

static uint64_t page_frame_base; // page frame起始
static uint64_t page_frame_end;  // page frame結束
static uint32_t page_frame_count;// 多少page frame

frame_entry *page_frame_entry;
list_head_t page_frame_freelist[MAX_ORDER];

uint32_t chunk_slot_size[] = {
    16, 32, 64, 128, 256, 512, 1024, 2048, 4096
};
chunk_slot_entry *chunk_slot_entries;
list_head_t chunk_slot_freelists[9];

void init_page_frame_allocator(void *start, void *end)
{
    page_frame_base = (uint64_t) start;
    page_frame_end = (uint64_t) end;
    page_frame_count = (page_frame_end - page_frame_base) / PAGE_SIZE;
    // allocate
    page_frame_entry = simple_malloc(page_frame_count * sizeof(frame_entry));
    // 給每個page frame 4K
    for(int i = 0; i < page_frame_count; i++){
    	page_frame_entry[i].order = 0;      // 2 ^ 0 * 4K = 4K
    	page_frame_entry[i].status = FREE;
    }
}

void init_page_frame_freelist(){
    for(int i = 0; i < MAX_ORDER; i++){
    	INIT_LIST_HEAD(&page_frame_freelist[i]);
    }
}

void init_page_frame_merge()
{
    for(int i = 0; i < MAX_ORDER - 1; i++){
    	int frame1 = 0, frame2 = 0;
    	while(1){
    	    // frame2是frame1的buddy order
    	    // frame2 = frame1 ^ (2 ^ order)
    	    frame2 = frame1 ^ (1 << i);
    	    // buddy count超過全部
    	    if(frame2 >= page_frame_count) break;
    	    // 兩frame都是沒被分配(free)且order(次方)相等就merge
    	    if(page_frame_entry[frame1].status == FREE && page_frame_entry[frame2].status == FREE && page_frame_entry[frame1].order == i && page_frame_entry[frame2].order == i){
    	    	++page_frame_entry[frame1].order;
    	    }
    	    // frame1 = 2^(i+1)
    	    int tmp = (1 << (i + 1));
    	    frame1 += tmp;
    	    if(frame1 >= page_frame_count) break;
    	}
    }
    
    init_page_frame_freelist();
    
    
    // 把merge後的page frame加到freelist裡
    for (int i = 0, odr = 0; i < page_frame_count; i += (1 << odr))
    {
        // i += (1 << odr)
        // because it need to skip its buddy
        // set freelist order
        odr = page_frame_entry[i].order;
        //uart_printf("odr : %d\n", odr);
        if (page_frame_entry[i].status == FREE)
        {
            frame_entry_list_head *tmp;
            tmp = idx2address(i);
            list_add_tail(&tmp->listhead, &page_frame_freelist[odr]);
#ifdef DEMO
            uart_printf("%d added to %d'th order list\n", i, odr);
#endif
        }
    }
}

void *page_frame_allocation(uint32_t page_num)
{
    // Time comeplexity : O(1), We only have to check the constant page frame freelist and get the page
    if (page_num == 0) return (void *)0;

    // get round up order
    page_num = ceiling(page_num);
    int order = power(page_num);   // page_num = 2 ^ order
    int alloc_page_order = order;

    // 是否有freelists, 無 則分配更大order然後再release
    while (list_empty(&page_frame_freelist[alloc_page_order])){
        ++alloc_page_order;
    }

    // 若跑到最大order, 則不分配
    if (alloc_page_order == MAX_ORDER){
        uart_async_printf("No page available !!! \n");
        return (void *)0;
    }

#ifdef DEMO
    uart_async_printf("original page order: %d\n", order);
    uart_async_printf("allocate page order: %d\n", alloc_page_order);
#endif

    // get allocated page index
    frame_entry_list_head *page_idx = (frame_entry_list_head *)page_frame_freelist[alloc_page_order].next;
    int index = address2idx(page_idx);

    // delete from free list
    list_del(&page_idx->listhead);

#ifdef DEMO
    uart_async_printf("allocate page index: %d\n", index);
#endif

    // Release redundant memory block
    while (alloc_page_order > order)
    {
        // use the block’s index xor with its exponent to find its buddy
        int buddy_idx = index ^ (1 << (--alloc_page_order));
        // set new order
        page_frame_entry[buddy_idx].order = alloc_page_order;
        page_frame_entry[buddy_idx].status = FREE;

#ifdef DEMO
    uart_async_printf("Released redundant page index: %d\n", buddy_idx);
    uart_async_printf("Released redundant page order: %d\n", alloc_page_order);
#endif
        // 再加回去freelist
        frame_entry_list_head *buddy_page_idx = idx2address(buddy_idx);
        list_add(&buddy_page_idx->listhead, &page_frame_freelist[alloc_page_order]);
    }

    // set origin size allocated page
    page_frame_entry[index].order = order;
    page_frame_entry[index].status = ALLOCATED;

#ifdef DEMO
    uart_async_printf("page is allocated at: %x\n", page_idx);
    uart_async_printf("--------------------------------------------\n");
#endif
    /*for(int i = 0; i < MAX_ORDER; i++){
    	uart_async_printf("page_frame_freelist[%d]: %d\n", i, page_frame_freelist[i].next);
    }*/
    return (void*)page_idx;
}

void page_frame_free(void *address){

    /*for(int i = 0; i < MAX_ORDER; i++){
    	uart_async_printf("page_frame_freelist[%d]: %d\n", i, page_frame_freelist[i]);
    }*/
#ifdef DEMO
    uart_async_printf("freeing: %x\n", address);
#endif

    // Time complexity : O(1), 從freelist中刪除entry, 並在const time merge page frame
    // get entry index
    frame_entry_list_head *page = (frame_entry_list_head *)address;
    int index = address2idx(page);
    int order = page_frame_entry[index].order;
    // free掉它並加入至freelist
    page_frame_entry[index].status = FREE;
    list_add(&page->listhead, &page_frame_freelist[order]);
    
#ifdef DEMO
    uart_async_printf("free page order: %d\n", order);
    uart_async_printf("free page index: %d\n", index);
#endif
    
    // buddy
    int buddy_idx = index ^ (1 << order);
    // merge buddy直到buddy不是free或有相同order
    while(order < MAX_ORDER - 1 && page_frame_entry[buddy_idx].status == FREE && page_frame_entry[buddy_idx].order == order){
    
#ifdef DEMO
    uart_async_printf("target page index: %d\n", index);
    uart_async_printf("buddy page index: %d\n", buddy_idx);
    uart_async_printf("page order: %d\n", order);
#endif     
        frame_entry_list_head *tmp;
        tmp = idx2address(index);
        list_del(&tmp->listhead);
        tmp = idx2address(buddy_idx);
        list_del(&tmp->listhead);
        
        index &= buddy_idx;
        //uart_async_printf("index: %d\n", index);
        page_frame_entry[index].order = ++order;
        tmp = idx2address(index);
        list_add(&tmp->listhead, &page_frame_freelist[order]);
        
        
        // 找下一個buddy
        buddy_idx = index ^ (1 << order);
    }
    /*for(int i = 0; i < MAX_ORDER; i++){
    	uart_async_printf("page_frame_freelist[%d]: %d\n", i, page_frame_freelist[i]);
    }*/


#ifdef DEMO
    uart_async_printf("--------------------------------------------\n");
#endif
}

void init_chunk_slot_allocator()
{
    // 跟page number一樣分配記憶體
    chunk_slot_entries = simple_malloc(page_frame_count * sizeof(chunk_slot_entry));
    for(int i = 0; i < page_frame_count; i++){
    	chunk_slot_entries[i].status = FREE;
    }
}

void init_chunk_slot_listhead()
{
    // 9種不同大小的chunk
    for (int i = 0; i < 9; ++i){
        INIT_LIST_HEAD(&chunk_slot_freelists[i]);
    }
}

void *chunk_slot_allocation(uint32_t size){
    // 找最小size的chunk slot
    int size_index = find_min_chunk_slot(size);
#ifdef DEMO
        uart_printf("alloc chunk slot size: %d\n", chunk_slot_size[size_index]);
#endif

    chunk_slot_list_head *cslh;
    if(list_empty(&chunk_slot_freelists[size_index])){
        void *page_address = page_frame_allocation(1);
        int page_index = address2idx(page_address);
#ifdef DEMO
        uart_printf("Split page %d into chunks of size %d\n", page_index, chunk_slot_size[size_index]);
#endif
        //把page切給free chunk
        chunk_slot_entries[page_index].size = size_index;
        chunk_slot_entries[page_index].status = ALLOCATED;
    
        for(int i = 0; i + chunk_slot_size[size_index] <= PAGE_SIZE; i += chunk_slot_size[size_index]){
            cslh = (chunk_slot_list_head *)((char *)page_address + i);
            list_add_tail(&cslh->listhead, &chunk_slot_freelists[size_index]);
        }
    }
    // 從free list刪除
    cslh = (chunk_slot_list_head *) chunk_slot_freelists[size_index].next;
    list_del(&cslh->listhead);
#ifdef DEMO
    uart_async_printf("chunk is allocated at: %x\n", cslh);
    uart_async_printf("--------------------------------------------\n");
#endif
    return cslh;
}

void chunk_slot_free(void *address)
{
    // get chunk index
    int page_index = address2idx(address);
    chunk_slot_list_head *cslh = (chunk_slot_list_head *)address;
    // 加到free list
    int size = chunk_slot_entries[page_index].size;
    list_add(&cslh->listhead, &chunk_slot_freelists[size]);

#ifdef DEMO
        uart_printf("Freeing %x chunk of size %d in page %d\n", address, chunk_slot_size[size], page_index);
#endif
}

void memory_reserve(void *start, void *end)
{
    // start & end address
    // start, end調整成PAGE_SIZE倍數
    start = (void *)((uint64_t) start / PAGE_SIZE * PAGE_SIZE); // ex : start = 5000, pagesize = 4096 , floor (align 0x1000)
    if ((uint64_t) end % PAGE_SIZE){				// ex : end = 4000, pagesize = 4096 , ceiling (align 0x1000)
    	end = (void *) ((uint64_t)end + PAGE_SIZE - ((uint64_t)end % PAGE_SIZE));
    }
    else end = (void *)((uint64_t) end);
    uart_async_printf("start : %d, end : %d\n\n", (uint64_t)start, (uint64_t)end);
    
    // 標記ALLOCATED已分配以下這些：
    // spin tables for multicore boot, initramfs, kernel image, startup allocation 
    for(void *i = start; i < end; i = (void *)((uint64_t)i + PAGE_SIZE)){
    	page_frame_entry[address2idx(i)].status = ALLOCATED;
    }
}

void memory_init()
{
    // lab給定memory region從0x00~0x3C000000
    init_page_frame_allocator((void *)0, (void *)0x3c000000);
    init_chunk_slot_allocator();

    uart_async_printf("memory_reserve for Spin tables for multicore boot:\n");
    // spin tables for multicore boot(0x0000, 0x1000)
    memory_reserve((void *)0, (void *)0x1000);
    
    uart_async_printf("memory_reserve for kernel image:\n");
    // heap and start kernel image 
    memory_reserve(&__text_start, &__heap_start);
    
    uart_async_printf("memory_reserve for initramfs:\n");
    // initramfs get from device tree
    memory_reserve(cpio_start, cpio_end);
    
    uart_async_printf("memory_reserve for startup allocator:\n");
    // simple allocator
    memory_reserve(&__startup_allocator_start, &__startup_allocator_end);

    // merge after reserve 
    init_page_frame_merge();
    init_chunk_slot_listhead();
}

void *malloc(uint32_t size)
{
    // 如果比page size小直接給chunk slot
    if (size < PAGE_SIZE){
#ifdef DEMO
        uart_async_printf("Allocate size: %d\n", size);
#endif
        return chunk_slot_allocation(size);
    }
    // 超過page size則分配
    else{
        uint32_t pn = (size + PAGE_SIZE - 1) / PAGE_SIZE;  // 從0~0x1000所以要扣1
        //uart_async_printf("%d", pn);
#ifdef DEMO
        uart_async_printf("Allocate size: %d, page num: %d\n", size, pn);
#endif
        return page_frame_allocation(pn);
    }
}

void page_frame_allocator_test()
{
    char *pages[10];

    uart_async_printf("Allocate pages[0]...\n");
    pages[0] = malloc(1*PAGE_SIZE + 123);
    uart_async_printf("Allocate pages[1]...\n");
    pages[1] = malloc(1*PAGE_SIZE);
    uart_async_printf("Allocate pages[2]...\n");
    pages[2] = malloc(3*PAGE_SIZE + 321);
    uart_async_printf("Allocate pages[3]...\n");
    pages[3] = malloc(1*PAGE_SIZE + 31);

    uart_async_printf("Allocate pages[4]...\n");
    pages[4] = malloc(1*PAGE_SIZE + 21);
    uart_async_printf("Allocate pages[5]...\n");
    pages[5] = malloc(1*PAGE_SIZE);

    uart_async_printf("Free pages[2]...\n");
    page_frame_free(pages[2]);
    uart_async_printf("Allocate pages[6]...\n");
    pages[6] = malloc(1*PAGE_SIZE);
    uart_async_printf("Allocate pages[7]...\n");
    pages[7] = malloc(1*PAGE_SIZE + 333);
    uart_async_printf("Allocate pages[8]...\n");
    pages[8] = malloc(1*PAGE_SIZE);


    // Merge blocks
    page_frame_free(pages[6]);
    page_frame_free(pages[7]);
    page_frame_free(pages[8]);

    // free all
    page_frame_free(pages[0]);
    page_frame_free(pages[1]);
    page_frame_free(pages[3]);
    page_frame_free(pages[4]);
    page_frame_free(pages[5]);
}

void chunk_slot_allocator_test(){
    char *chunk[100];
    
    int tmp = PAGE_SIZE / 512;
    // allocated page
    // use all chunk slot of allocated page
    for (int i = 0; i <= 8; ++i){
        chunk[i] = malloc(0x101);
    }
    // then it will ask another page
    chunk[9] = malloc(0x11);
    chunk[10] = malloc(0x15);
    chunk[11] = malloc(0x25);
    chunk[12] = malloc(0x35);

    for (int i = 0; i <= 8; ++i){
        chunk_slot_free(chunk[i]);
    }
    chunk_slot_free(chunk[9]);
    chunk_slot_free(chunk[10]);
    chunk_slot_free(chunk[11]);
    chunk_slot_free(chunk[12]);
    
}

/* Utility functions */

uint32_t power(uint32_t x)
{
    return (x > 1) ? 1 + power(x / 2) : 0;
}

uint32_t address2idx(void *address)
{
    return ((uint64_t)address - page_frame_base) / PAGE_SIZE;
}

void *idx2address(uint32_t idx)
{
    return (void *)(page_frame_base + idx * PAGE_SIZE);
}

int find_min_chunk_slot(uint32_t size)
{
    for (uint32_t i = 0 ; i < 9; ++i){
        if (chunk_slot_size[i] >= size) return i;
    }
    return -1;
}

uint32_t ceiling(uint32_t number){
	int ceiling = 1;
	while(ceiling < number){
		ceiling <<= 1;
	}
	return ceiling;
}	
