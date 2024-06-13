#include "memory.h"
#include "list.h"
#include "uart1.h"
#include "exception.h"
#include "dtb.h"
#include "cpio.h"
#include "mmu.h"

extern char _heap_start;
static char *htop_ptr = &_heap_start;

extern char _start;
extern char _end;
extern char _stack_end;
extern char _stack_top;

//static char *htop_ptr = &_end;

#ifdef DEBUG
    #define memory_sendline(fmt, args ...) uart_sendline(fmt, ##args)
#else
    #define memory_sendline(fmt, args ...) (void)0
#endif

void *s_allocator(unsigned int size)
{
    // void *allocated = (void *)htop_ptr;
    // size = size % 16 == 0? size: size + (16 - (size % 16));
    // htop_ptr += size;
    // return allocated;
    // 0x10 for heap_block header
    char* r = htop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned int*)(r - 0x8) = size;
    htop_ptr += size;
    return r;
}

void s_free(void *ptr)
{
    // TBD
}

// ------ Lab4 ------

static page_t *page_array;
static list_head_t free_page_list[PAGE_MAX_IDX + 1]; // index=0-6
static list_head_t slab_list[SLAB_MAX_IDX + 1];      // 0-7

void init_memory()
{

    page_array = s_allocator(BUDDY_MEMORY_PAGE_COUNT * sizeof(page_t));

    // init free list
    for (int i = 0; i <= PAGE_MAX_IDX; i++)
    {
        INIT_LIST_HEAD(&free_page_list[i]);
    }
    // init slab list
    for (int i = 0; i <= SLAB_MAX_IDX; i++)
    {
        INIT_LIST_HEAD(&slab_list[i]);
    }
    // init page array: 2^6的list中 每個連續page的第一個page 6=>最多的連續page 初始時只有2^6的list有
    // 初始每個page
    for (int i = 0; i < BUDDY_MEMORY_PAGE_COUNT; i++)
    {
        INIT_LIST_HEAD(&page_array[i].listhead);
        page_array[i].idx = i;
        page_array[i].slab_order = SLAB_NONE;
        if (i % (1 << (PAGE_MAX_IDX)) == 0)
        {
            page_array[i].order = PAGE_MAX_IDX;
            page_array[i].used = PAGE_FREE;
            list_add(&page_array[i].listhead, &free_page_list[PAGE_MAX_IDX]);
        }
    }
    // print_slab_info();
    // print_page_info();
    // while(1){}

    //  setting, print
    memory_sendline("Startup Allocation\n");
    memory_sendline("\nDTB\n");
    dtb_reserved_memory();
    memory_sendline("\nPGD, PUD, PMD\n");
    memory_reserve(PHYS_TO_VIRT(MMU_PGD_ADDR), PHYS_TO_VIRT(MMU_PMD_ADDR+0x2000)); // // PGD's page frame at 0x1000 // PUD's page frame at 0x2000 PMD 0x3000-0x5000
    memory_sendline("\nkernel\n");
    memory_reserve((unsigned long long)&_start, (unsigned long long)&_end); // kernel
    memory_sendline("\nheap\n");
    memory_reserve((unsigned long long)&_stack_end, (unsigned long long)&_stack_top); // heap & stack -> simple allocator
    memory_sendline("\nCPIO\n");
    memory_reserve((unsigned long long)CPIO_START, (unsigned long long)CPIO_END);
}

void *page_malloc(unsigned int size)
{
    //  print
    memory_sendline("Before allocate new pages\n");
    print_page_info();
    int order, i;
    // 找到適當的page數量 2^order連續pages
    for (i = 0; i < (PAGE_MAX_IDX + 1); i++)
    {
        if (size <= (PAGESIZE << i)) // page size * 2^i
        {
            order = i;
            break;
        }
    }
    // size over 2^6 pages
    if (i > PAGE_MAX_IDX)
    {
        memory_sendline("Page Malloc Error: Size over the biggest continous pages\n");
        return (void *)INVALID_ADDRESS;
    }

    int big_order; // 找到可以使用的連續page, big_order >= order,
    for (big_order = order; big_order < PAGE_MAX_IDX + 1; big_order++)
    {
        if (!list_empty(&free_page_list[big_order]))
            break;
    }
    if (big_order > PAGE_MAX_IDX)
    {
        memory_sendline("Page Malloc Error: No free continuous pages\n");
        return (void *)INVALID_ADDRESS;
    }

    // get the pointer to target page
    page_t *new_ptr = (page_t *)free_page_list[big_order].next;

    if (big_order > order) // 為了印出page數量是對的，因為要先把page從free page list刪掉
    {
        memory_sendline("\n\tSpliting...\n\tBefore spliting\n");
        print_page_info();
    }
    list_del_entry((list_head_t *)new_ptr);
    for (i = big_order; i > order; i--)
    {
        if (i != big_order) // 為了印出page數量是對的，因為要先把page從free page list刪掉
        {
            memory_sendline("\n\tSpliting...\n\tBefore spliting\n");
            print_page_info();
        }
        split_buddy(new_ptr);
    }

    new_ptr->used = PAGE_ALLOCATED;
    memory_sendline("After allocate new pages\n");
    print_page_info();
    return (void *)BUDDY_MEMORY_BASE + (PAGESIZE * new_ptr->idx);
}

void page_free(void *ptr)
{
    //  2^12 = 4096(4KB) page index = (ptr-buddy base mem) / page size
    page_t *page = &page_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12];
    if (page->used == PAGE_FREE)
    {
        memory_sendline("Free error: These pages have already been freed\n");
        return;
    }

    // : print
    memory_sendline("Before free pages\n");
    print_page_info();

    // int tmp = merge_buddy(page);
    // while (tmp != INVALID_ADDRESS)
    // {
    //     page_array[tmp].used = PAGE_FREE;
    //     tmp = merge_buddy(page);
    // }
    while(merge_buddy(page) == 0){
        page->used = PAGE_FREE;
    }
    
    //*page = page_array[tmp];
    page->used = PAGE_FREE;
    // uart_sendline("%d\n", page->idx);
    list_add(&page->listhead, &free_page_list[page->order]);

    // : print
    memory_sendline("After free pages\n");
    print_page_info();
}

// 找所在order的buddy
page_t *find_buddy(page_t *page)
{
    return &page_array[page->idx ^ (1 << page->order)];
}

// split pages into 2 1/2pages
page_t *split_buddy(page_t *page)
{
    unsigned int b_start_idx = page->idx;
    unsigned int b_end_idx = b_start_idx + pow(2, page->order) - 1;

    page->order -= 1;
    page_t *buddy = find_buddy(page);
    buddy->order = page->order;
    list_add(&buddy->listhead, &free_page_list[buddy->order]);
    memory_sendline("\tAfter spliting\n");
    print_page_info();

    unsigned int af_start_idx = page->idx;
    unsigned int af_end_idx = af_start_idx + pow(2, page->order) - 1;
    unsigned int as_start_idx = buddy->idx;
    unsigned int as_end_idx = as_start_idx + pow(2, buddy->order) - 1;

    memory_sendline("\tSplit from %d ~ %d to %d ~ %d and %d to %d\n\n", b_start_idx, b_end_idx, af_start_idx, af_end_idx, as_start_idx, as_end_idx);
    memory_sendline("\t=========================\n");
    return page;
}

int merge_buddy(page_t *page)
{

    page_t *buddy = find_buddy(page);
    memory_sendline("\nMerging...\nBefore merging\n");
    memory_sendline("order = %d, index = %d, used = %s, buddy order = %d, buddy index = %d, buddy used = %s\n", page->order, page->idx, page->used==PAGE_FREE?"Free":"Allocated", buddy->order, buddy->idx, buddy->used==PAGE_FREE?"Free":"Allocated");

    // page的order已經最大, page order跟buddy order不同, buddy正在被使用 => 不可merge
    if (page->order == PAGE_MAX_IDX || page->order != buddy->order || buddy->used == PAGE_ALLOCATED){
        memory_sendline("Merge error: can't merge pages\n");
        return INVALID_ADDRESS;
    }
    
    if (page->idx < buddy->idx)
    {
    }
    else if (page->idx > buddy->idx)
    {
        page_t *tmp = buddy;
        memory_sendline("tmp = %d ", tmp->idx);
        buddy = page;
        memory_sendline("buddy = %d ", buddy->idx);
        page = tmp;
        memory_sendline("page = %d\n", page->idx);
    }
    else
    {
        memory_sendline("Merge error: can't merge pages!\n");
        return INVALID_ADDRESS;
    }
    //uart_sendline("swaped\n");
    list_del_entry((list_head_t *)buddy);
    page->order += 1;

    memory_sendline("After merging\n");
    memory_sendline("order = %d, index = %d\n", page->order, page->idx);

    return 0;
}

void get_new_slab(int order)
{
    memory_sendline("Get new slabs from a page\n");
    // print order and address
    char *page = page_malloc(PAGESIZE);
    page_t *page_ptr = &page_array[((unsigned long long)page - BUDDY_MEMORY_BASE) >> 12];
    page_ptr->slab_order = order;

    int slab_size = (16 << order); // smallest slab size = 16B
    for (int i = 0; i < PAGESIZE; i += slab_size)
    {
        // split a page into slabs and store in list
        // 從第0個slab開始加到list的頭
        // max_idx, max_idx-1, ... ,1 ,0
        list_head_t *tmp = (list_head_t *)(page + i);
        list_add(tmp, &slab_list[order]);
    }

    memory_sendline("Partition idx = %d page (0x%x) into slabs\n", page_ptr->idx, (unsigned long long)page);
}

// 一開始全部都是空的，如有需要就從page切成同等大小的slab存入該order的slab list
void *slab_malloc(unsigned int size)
{
    // : print
    memory_sendline("Before allocate new slabs\n");
    print_slab_info();
    int order;
    for (int i = 0; i < SLAB_MAX_IDX + 1; i++)
    {
        if (size <= (16 << i))
        {
            order = i;
            break;
        }
    }
    // uart_sendline("order: %d\n");
    // no slab can use in the order
    if (list_empty(&slab_list[order]))
    {
        get_new_slab(order);
    }

    list_head_t *slab = slab_list[order].next;
    list_del_entry(slab);

    // : print
    memory_sendline("After allocate new slabs\n");
    print_slab_info();
    return slab;
}

void slab_free(void *ptr)
{
    list_head_t *tmp = (list_head_t *)ptr;
    // 透過address取的切成此大小slab的page number
    page_t *page = &page_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12];

    // : print
    memory_sendline("Before free slabs\n");
    print_slab_info();
    list_add(tmp, &slab_list[page->slab_order]);

    // : print
    memory_sendline("After free slabs\n");
    print_slab_info();
}

void *kmalloc(unsigned int size)
{
    lock();
    memory_sendline("\nRequest for size: %d\n", size);
    void *address;
    // size > 最大的slab size(2048B = 16B*2^7) give pages
    if (size > (16 << SLAB_MAX_IDX))
    {
        address = page_malloc(size);
    }
    // give slabs
    else
    {
        address = slab_malloc(size);
    }
    if((unsigned long long)address != INVALID_ADDRESS)
        memory_sendline("Return avalible address: 0x%x\n\n", address);
    unlock();
    return address;
}

void kfree(void *ptr)
{
    lock();
    memory_sendline("\nFree address: 0x%x\n", ptr);
    if((unsigned long long)ptr == INVALID_ADDRESS){
        memory_sendline("Free error: It's not a valid address.\n");
        unlock();
        return;
    }
    // using pages
    if (page_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12].slab_order == SLAB_NONE)
    {
        page_free(ptr);
        unlock();
        return;
    }
    slab_free(ptr);
    unlock();
}

/*
1. mark reserved pages
2. splt the pages if it contain reserved pages
*/
void memory_reserve(unsigned long long start, unsigned long long end)
{
    // start align multiple of page size
    // start -= start % PAGESIZE;
    // //  end align to mulitiple of page size
    // if (end % PAGESIZE != 0)
    // {
    //     end += PAGESIZE - (end % PAGESIZE);
    // }

    // find the page contain start-end
    unsigned start_idx = start >> 12;
    unsigned end_idx = end >> 12;
    uart_sendline("\nReserve memory form 0x%x (%d) ~ 0x%x (%d)\n", start, start_idx, end, end_idx);
    memory_sendline("Before reserving...\n");
    print_page_info();
    // uart_sendline("start_idx: %d  end_idx: %d\n", start_idx, end_idx);
    for (int i = start_idx; i <= end_idx; i++)
    {
        page_array[i].used = PAGE_ALLOCATED;
    }

    for (int order = PAGE_MAX_IDX; order >= 0; order--)
    {
        list_head_t *cur;
        list_for_each(cur, &free_page_list[order])
        {
            unsigned int start_page_idx = ((page_t *)cur)->idx;
            unsigned int end_page_idx = start_page_idx + pow(2, order) - 1;
            // uart_sendline("start_idx: %d  end_idx: %d  cur: %d\n", start_idx, end_idx, ((page_t *)cur)->idx);
            // uart_sendline("start_page_idx: %d  end_page_idx: %d  order: %d\n", start_page_idx, end_page_idx, order);
            // start <= start_page < end_page <= end
            if (start_idx <= start_page_idx && end_page_idx <= end_idx)
            {
                unsigned long long start_addr = start_page_idx << 12;
                unsigned long long end_addr = ((end_page_idx + 1) << 12) - 1;
                memory_sendline("\t[%d]Reserving 0x%x (%d) ~ 0x%x (%d)\n", order, start_addr, start_page_idx, end_addr, end_page_idx);
                memory_sendline("\tBefore reserving...\n");
                print_page_info();
                list_del_entry(cur);
                memory_sendline("\tAfter reserving...\n");
                print_page_info();
                continue;
            }
            // no overlap
            else if (start_idx > end_page_idx || end_idx < start_page_idx)
            {
                continue;
            }
            // if contain reserved pages, split and check next round(order-1)
            else
            {
                list_head_t *tmp;
                tmp = cur->prev;
                memory_sendline("\n\tSpliting...\n\tBefore spliting\n");
                print_page_info();
                list_del_entry(cur);
                //  buddy在split_buddy中被加入order-1的list
                //  將cur也加入
                list_add(&split_buddy((page_t *)cur)->listhead, &free_page_list[order - 1]);
                cur = tmp;
            }
        }

        // uart_sendline("order: %d\n", order);
    }
    memory_sendline("\nAfter reserving...\n");
    print_page_info();
}

int pow(int base, int exponent)
{
    int result = 1;
    for (; exponent > 0; exponent--)
    {
        result = result * base;
    }
    return result;
}

void print_page_info()
{
    memory_sendline("\t[Pages info]: \n\t");
    for (int i = 0; i <= PAGE_MAX_IDX; i++)
    {
        memory_sendline("%dKB (Order: %d): %d | ", 4 * pow(2, i), i, list_size(&free_page_list[i]));
    }
    memory_sendline("\n\t===============================\n");
}

void print_slab_info()
{
    memory_sendline("\t[Slabs info]: \n\t");
    for (int i = 0; i <= SLAB_MAX_IDX; i++)
    {
        memory_sendline("%dB (Order: %d): %d | ", 16 * pow(2, i), i, list_size(&slab_list[i]));
    }
    memory_sendline("\n\t===============================\n");
}