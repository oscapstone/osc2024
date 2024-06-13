#include "memory.h"
#include "cpio.h"
#include "mini_uart.h"
#include "utility.h"
#include "stdint.h"

static page_frame_t *frame_array;
static list_head_t frame_freelist[FRAME_MAX_IDX];
static list_head_t cache_list[CACHE_IDX_FINAL + 1];

extern unsigned long long __begin;
extern unsigned long long __end;
extern unsigned long long __bss_start;
extern unsigned long long __bss_end;
extern unsigned long long __heap_top;
extern unsigned long long __heap_bottom;
extern unsigned long long __stack_top;

static inline page_frame_t *phy_addr_to_frame(void *ptr)
{
    return (page_frame_t *)&frame_array[(unsigned long)(ptr - BUDDY_SYSTEM_BASE) / PAGE_SIZE];
}
static inline void *frame_to_phy_addr(page_frame_t *pg_t)
{
    return BUDDY_SYSTEM_BASE + PAGE_SIZE * pg_t->idx;
}
void init_memory_space()
{
    // it seems that I have to allocate a space for this frame_array

    frame_array = heap_malloc(BUDDY_SYSTEM_PAGE_COUNT * sizeof(page_frame_t));
    uart_hex(frame_array);
    uart_puts("\r\n");

    // First, initial the linked-list for each order size
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
    {
        INIT_LIST_HEAD(&frame_freelist[i]);
    }
    uart_puts("\n[1] Frame Freelist Init !\n");
    pg_info_dump();

    for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++)
    {
        INIT_LIST_HEAD(&cache_list[i]);
    }
    uart_puts("[2] Cache List Init !!\n");
    cache_info_dump();

    // Subquentently, initial frame array

    for (int i = 0; i < BUDDY_SYSTEM_PAGE_COUNT; i++)
    {
        if (i % (1 << FRAME_IDX_FINAL) == 0)
        {
            frame_array[i].order = FRAME_IDX_FINAL;
            frame_array[i].status = FREE;
        }
        else
        {
            frame_array[i].order = FRAME_IDX_FINAL;
            frame_array[i].status = isBelonging;
        }
        INIT_LIST_HEAD(&frame_array[i].listhead);
        //  Finally, set each page frame a index number
        frame_array[i].idx = i;
        frame_array[i].cache_order = CACHE_NONE;

        // add init frame (FRAME_IDX_FINAL) into freelist
        if (i % (1 << FRAME_IDX_FINAL) == 0)
        {
            list_add(&frame_array[i].listhead, &frame_freelist[FRAME_IDX_FINAL]);
        }
    }
    uart_puts("[3] Page Frame Array Init !!!\n");
    pg_info_dump();

    uart_puts("[4] Memory Reserve Spin tables for multicore boot\n");
    memory_reserve(0x0000, 0x1000);
    pg_info_dump();

    uart_puts("[5] Memory Reserve Kernel image in the physical memory\n");
    memory_reserve(&__begin, &__end);
    pg_info_dump();

    uart_puts("[6] Memory Reserve Your simple allocator\n");
    memory_reserve(&__heap_top, &__stack_top);
    pg_info_dump();

    uart_puts("[7] Memory Reserve Initramfs\n");
    memory_reserve(CPIO_PLACE, CPIO_PLACE + 0x1000);

    pg_info_dump();
}

void *page_alloc(unsigned int size)
{
    // pg_info_dump();
    /*給一個size，開始去便利所有order找到位於哪兩個order之間 設 x < size < y，
    檢查order y的freelist有無空間

    有：

    沒有：

        */
    // uart_puts("[+] Page Allocate \r\n");
    int order;
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
    {
        if (size <= (PAGE_SIZE << i))
        {
            order = i;
            // uart_puts("[+] This malloc acquires ");
            // put_int(order);
            // uart_puts(" page frame. \r\n");
            break;
        }

        if (i == FRAME_MAX_IDX)
        {
            // uart_puts(" [!] Request size out of memory in page malloc \r\n");
            return (void *)0;
        }
    }

    int target_order = order;
    page_frame_t *target_frame_ptr;
    if (list_empty(&frame_freelist[target_order]))
    {
        target_frame_ptr = split_to_target_freelist(target_order);
    }
    else
    {
        target_frame_ptr = frame_freelist[target_order].next;
    }

    // 將此快page frame設為allocated
    if (target_frame_ptr != 1)
    {
        // 不能切的情況再說
    }
    target_frame_ptr->status = Allocated;

    // uart_puts("Here the page start :");
    // uart_hex(target_frame_ptr);
    // uart_puts("\r\n");

    list_del_entry(target_frame_ptr);
    // pg_info_dump();
    return frame_to_phy_addr(target_frame_ptr);
};
page_frame_t *split_to_target_freelist(int order)
{
    // uart_puts("[+] Split !!\r\n");
    int available_order = -1;
    // 給好一塊page frame之後，將page frame包到的page frame從對應order的free list刪除
    for (int i = order; i <= FRAME_MAX_IDX; i++)
    {
        if (!list_empty(&frame_freelist[i]))
        {
            available_order = i;
            break;
        }
    }
    if (available_order == -1)
    {
        return (void *)1; // define magic number
    }

    page_frame_t *target_frame_ptr;
    target_frame_ptr = (page_frame_t *)frame_freelist[available_order].next;
    list_del_entry((struct list_head *)target_frame_ptr);
    for (int j = available_order; j > order; j--)
    {
        target_frame_ptr->order--;
        page_frame_t *buddy_frame_ptr = find_buddy(target_frame_ptr);
        list_add(buddy_frame_ptr, &frame_freelist[j - 1]);
        buddy_frame_ptr->status = FREE;
        buddy_frame_ptr->order = j - 1;
    }
    // put_int(target_frame_ptr->order);
    return target_frame_ptr;
}

void page_free(void *ptr)
{
    // pg_info_dump();
    // uart_puts("[+] Free a frame which order is ");
    // put_int(phy_addr_to_frame(ptr)->order);
    // uart_puts("\r\n");
    page_frame_t *pg_ptr = phy_addr_to_frame(ptr);
    while (buddy_can_merge(pg_ptr))
    {
        pg_ptr = merge(pg_ptr);
    }
    pg_ptr->status = FREE;
    // 指到指定區塊 將指向的page frame status設定為 FREE
    // 將此區塊加回去對應大小的free list
    list_add(&pg_ptr->listhead, &frame_freelist[pg_ptr->order]);
    // pg_info_dump();
};

page_frame_t *find_buddy(page_frame_t *pg_ptr)
{
    return &frame_array[pg_ptr->idx ^ (1 << pg_ptr->order)];
};
enum results buddy_can_merge(page_frame_t *pg_ptr)
{

    if (pg_ptr->order == FRAME_IDX_FINAL)
    {
        return Fault;
    }
    page_frame_t *buddy_ptr = find_buddy(pg_ptr);

    // 如果 buddy 和 page不同 order 不可以合併
    // 如果buddy 被allocated 不可以合併

    if (buddy_ptr->order != pg_ptr->order || buddy_ptr->status == Allocated)
    {
        // uart_puts("buddy can't merge\r\n");
        return Fault;
    }
    // uart_puts("buddy can merge \r\n");
    return Success;
}
page_frame_t *merge(page_frame_t *pg_ptr)
{
    // 從pg ptr 找出他的buddy
    page_frame_t *buddy_ptr = find_buddy(pg_ptr);

    list_del_entry(buddy_ptr);
    // uart_puts("[+] Merge ~~\r\n");

    page_frame_t *target_ptr;
    // 如果可以合併 pg 和 buddy 取最小的address作為放進freelist
    if (pg_ptr > buddy_ptr)
    {
        target_ptr = buddy_ptr;
        // 右側的page frame->status = isBelonging
        pg_ptr->status = isBelonging;
    }
    else
    {
        target_ptr = pg_ptr;
        buddy_ptr->status = isBelonging;
    }

    // 左側的page frame->status = FREE
    // 左側的page frame->order = 原本order + 1
    // 左側的page frame 加入到freelist
    target_ptr->status = FREE;
    // put_int(target_ptr->order);
    target_ptr->order++;

    return target_ptr;
};

void pg_info_dump()
{
    uart_puts("\n|-------------------------------Page Info--------------------------------|\r\n");
    uart_puts("|    order|     amout|    freelist|       loca1|       loca2|       loca3|\r\n");
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
    {
        uart_puts("|        ");
        put_int(i);

        uart_puts("|");
        int listSize = list_size(&frame_freelist[i]);
        // put_int(listSize);
        for (int i = 10000000000; i > listSize + 1; i = i / 10)
        {
            uart_puts(" ");
            // put_int(i);
        }
        put_int(list_size(&frame_freelist[i]));

        uart_puts("|    ");
        uart_hex(&frame_freelist[i]);
        uart_puts("|    ");
        uart_hex((frame_freelist[i].next));
        uart_puts("|    ");
        uart_hex((frame_freelist[i].next)->next);
        uart_puts("|    ");
        uart_hex(((frame_freelist[i].next)->next)->next);

        uart_puts("|\r\n");
    }
    uart_puts("|------------------------------End Page Info-----------------------------|\r\n");
    uart_puts("\r\n");
}

void *cache_alloc(unsigned int size)
{
    //  看要給多大的cache，
    // cache_info_dump();
    // uart_puts("[+] Cache Allocate \r\n");
    int order;
    for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++)
    {
        if (size <= (CACHE_SIZE << i))
        {
            order = i;
            // uart_puts("[+] This malloc acquires ");
            // put_int(order);
            // uart_puts(" cache size. \r\n");
            break;
        }
    }

    int target_order = order;
    if (list_empty(&cache_list[target_order]))
    {
        // uart_puts("[+] Need a Page ~\r\n");
        page_to_cache_pool(target_order);
    }
    list_head_t *target_cache_ptr = cache_list[target_order].next;
    page_frame_t *target_cache_pg = phy_addr_to_frame(target_cache_ptr);
    target_cache_pg->cache_used_count--;
    // uart_puts("[=] The cache start : ");
    // uart_hex(cache_list[target_order].next);
    list_del_entry(target_cache_ptr);
    // cache_info_dump();

    return target_cache_ptr;
}

void page_to_cache_pool(int cache_order)
{
    // assign a page frame for seperating into cache pool
    char *page = page_alloc(PAGE_SIZE);
    // uart_puts("page start for cache : ");
    // uart_hex(page);
    // uart_puts("\r\n");
    page_frame_t *page_for_cache = phy_addr_to_frame(page);
    page_for_cache->cache_order = cache_order;
    // uart_puts("page_for_cache : ");
    // uart_hex(page_for_cache);
    // uart_puts("\r\n");

    int cache_start = frame_to_phy_addr(page_for_cache);
    // uart_puts("cache_size : ");
    // uart_hex(cache_start);
    // uart_puts("\r\n");

    int cache_size = (CACHE_SIZE << cache_order);
    // uart_puts("cache_size : ");
    // uart_hex(cache_size);
    // uart_puts("\r\n");

    for (int i = 0; i < PAGE_SIZE; i += cache_size)
    {
        // uart_puts("[!] Current Cache ptr : ");
        // uart_hex(i);
        // uart_puts("\r\n");
        list_head_t *chunk = (list_head_t *)(cache_start + i);
        // uart_puts("[*] Here the chunk start :");
        // uart_hex(chunk);
        // uart_puts("\r\n");
        list_add(chunk, &cache_list[cache_order]);
    }
    page_for_cache->cache_used_count = PAGE_SIZE / cache_size;
}
void cache_free(void *cache_ptr)
{
    // cache_info_dump();

    list_head_t *free_cache_ptr = (list_head_t *)cache_ptr;
    page_frame_t *cache_pg_frame = &frame_array[((unsigned long long)cache_ptr - BUDDY_SYSTEM_BASE) >> 12];
    int cache_size_order = cache_pg_frame->cache_order;
    list_add(free_cache_ptr, &cache_list[cache_size_order]);
    cache_pg_frame->cache_used_count++;

    // uart_puts("[-] Free a cache ~ which cacher order is ");
    // put_int(cache_size_order);
    // uart_puts("\r\n");

    int cache_used_count = cache_pg_frame->cache_used_count;
    int cache_size = (1 << (cache_pg_frame->cache_order + 4));
    if (cache_used_count * cache_size == PAGE_SIZE)
    {
        // uart_hex(frame_to_phy_addr(cache_pg_frame));
        // uart_puts("\r\nThe Cache list of order ");
        // put_int(cache_pg_frame->cache_order);
        // uart_puts(" has ");
        // put_int(cache_used_count);
        // uart_puts(" elements.\r\n");

        for (int i = 0; i < cache_used_count; i++)
        {
            // if (!list_is_head(&cache_list[cache_size_order], cache_list[cache_size_order].next))
            // {
            //     list_del_entry(cache_list[cache_size_order].next);
            // }
            list_del_entry((list_head_t *)((void *)frame_to_phy_addr(cache_pg_frame) + i * cache_size));
        }

        cache_pg_frame->cache_order = CACHE_NONE;
        cache_pg_frame->cache_used_count = 0;
        
        // pg_info_dump();
        // uart_puts("[+] Cache merge into Page ~ \r\n");
        // uart_hex(frame_to_phy_addr(cache_pg_frame));
        page_free(frame_to_phy_addr(cache_pg_frame));
        // pg_info_dump();
    }

    // cache_info_dump();
}

void cache_info_dump()
{
    uart_puts("\n|=============================Cache Info================================|\r\n");
    uart_puts("|                             order|amout                               |\r\n");
    uart_puts("|=======================================================================|\r\n");
    for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++)
    {
        uart_puts("|                                 ");
        put_int(i);
        uart_puts("|");
        put_int(list_size(&cache_list[i]));

        int listSize = list_size(&cache_list[i]);
        if (listSize <10){
            uart_puts("                                   |\r\n");
        } else if (listSize >= 10 && listSize < 100){
            uart_puts("                                  |\r\n");
        } else if (listSize >=100 ){
            uart_puts("                                 |\r\n");
        }
    }
    uart_puts("|============================End Cache Info=============================|\r\n");
    uart_puts("\r\n");
}

void memory_reserve(unsigned long long start, unsigned long long end)
{
    // 把 start 和 end地址 去對齊 PAGESIZE的倍數
    unsigned long long padding_start = start - start % PAGE_SIZE;
    unsigned long long padding_end = end % PAGE_SIZE ? end + PAGE_SIZE - (end % PAGE_SIZE) : end; // ceiling (align 0x1000)

    uart_puts("[$] Memory Reserve: Start from 0x");
    uart_hex(padding_start);
    uart_puts(" to the End 0x");
    uart_hex(padding_end);
    uart_puts("\r\n");

    // delete page from free list  not by myself yet
    for (int order = FRAME_IDX_FINAL; order >= 0; order--)
    {
        list_head_t *pos;

        list_for_each(pos, &frame_freelist[order])
        {
            unsigned long long pagestart = frame_to_phy_addr(pos); //((page_frame_t *)pos)->idx * PAGE_SIZE + BUDDY_SYSTEM_BASE;
            unsigned long long pageend = pagestart + (PAGE_SIZE << order);

            if (padding_start <= pagestart && padding_end >= pageend) // if page all in reserved memory -> delete it from freelist
            {
                ((page_frame_t *)pos)->status = Allocated;
                list_del_entry(pos);
            }
            else if (padding_start >= pageend || padding_end <= pagestart) // no intersection
            {
                continue;
            }
            else // partial intersection, separate the page into smaller size.
            {
                list_del_entry(pos);
                list_head_t *temppos = pos->prev;
                // 剩下的去切 然後Allocated 重複到完全被貼完畢
                split_to_target_freelist(order - 1); //???
                pos = temppos;
            }
        }
    }
};

void *kmalloc(unsigned int size){
    void * ptr;
    if (size >= PAGE_SIZE){
        ptr = page_alloc(size);
        return ptr;
    } else if (size < PAGE_SIZE){
        ptr = cache_alloc(size);
        return ptr;
    }
}

void kfree(void *ptr){
    // while(1){};
    lock();
    page_frame_t *frame = phy_addr_to_frame(ptr);

    if (frame->cache_order == CACHE_NONE){
        page_free(ptr);
        unlock();
        return;
    } else {
        cache_free(ptr);
        unlock();
        return;
    }

    unlock();
    return;
}