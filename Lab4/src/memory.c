#include "memory.h"
#include "u_list.h"
#include "uart.h"
#include "exception.h"
#include "dtb.h"

extern char _heap_top;
static char *htop_ptr = &_heap_top;

extern char _start;
extern char _end;
extern char _stack_top;
extern char *CPIO_START;
extern char *CPIO_END;
extern char *dtb_ptr;

//static char *htop_ptr = &_end;

void *s_allocator(unsigned int size)
{
    void *allocated = (void *)htop_ptr;
    htop_ptr += size;
    return allocated;
}

void s_free(void *ptr)
{
    
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
    uart_sendline("Startup Allocation\n");
    //https://community.arm.com/support-forums/f/architectures-and-processors-forum/44183/setting-pc-for-and-waking-up-secondary-cores-from-the-primary-core
    uart_sendline("\nSpin tables for multicore boot\n");
    memory_reserve((unsigned long long)0x0000, (unsigned long long)0x1000);
    uart_sendline("\nKernel image in the physical memory\n");
    memory_reserve((unsigned long long)&_start, (unsigned long long)&_end); // kernel
    //uart_sendline("\nheap\n");
    //memory_reserve((unsigned long long)&_heap_top, (unsigned long long)&_stack_top); // heap & stack -> simple allocator
    uart_sendline("\nInitramfs\n");
    memory_reserve((unsigned long long)CPIO_START, (unsigned long long)CPIO_END);
    uart_sendline("\nDevicetree \n");
    dtb_reserved_memory();
    //uart_sendline("\nBuddy System\n");
    //unsigned long long page_end = (unsigned long long)&page_array[BUDDY_MEMORY_PAGE_COUNT-1];
    //memory_reserve((unsigned long long)&page_array, page_end);
}

void *page_malloc(unsigned int size)
{
    //  print
    uart_sendline("Before allocate new pages\n");
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
        uart_sendline("Page Malloc Error: Size over the biggest continous pages\n");
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
        uart_sendline("Page Malloc Error: No free continuous pages\n");
        return (void *)INVALID_ADDRESS;
    }

    // get the pointer to target page
    page_t *new_ptr = (page_t *)free_page_list[big_order].next;

    if (big_order > order) // 為了印出page數量是對的，因為要先把page從free page list刪掉
    {
        uart_sendline("\n\tSpliting...\n\tBefore spliting\n");
        print_page_info();
    }
    list_del_entry((list_head_t *)new_ptr);
    for (i = big_order; i > order; i--)
    {
        if (i != big_order) // 為了印出page數量是對的，因為要先把page從free page list刪掉
        {
            uart_sendline("\n\tSpliting...\n\tBefore spliting\n");
            print_page_info();
        }
        split_buddy(new_ptr);
    }

    new_ptr->used = PAGE_ALLOCATED;
    uart_sendline("After allocate new pages\n");
    print_page_info();
    return (void *)BUDDY_MEMORY_BASE + (PAGESIZE * new_ptr->idx);
}

void page_free(void *ptr)
{
    //  2^12 = 4096(4KB) page index = (ptr-buddy base mem) / page size
    page_t *page = &page_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12];
    if (page->used == PAGE_FREE)
    {
        uart_sendline("Free error: These pages have already been freed\n");
        return;
    }

    // : print
    uart_sendline("Before free pages\n");
    print_page_info();

    int tmp = merge_buddy(page);
    // while (tmp != INVALID_ADDRESS)
    // {
    //     page_array[tmp].used = PAGE_FREE;
    //     tmp = merge_buddy(page);
    // }
    while(tmp != -1){
        if(tmp == 1){
            page_t *buddy = find_buddy(page);
            swap(&page, &buddy);
        }
        page->used = PAGE_FREE;
        // uart_sendline("After merging\n");
        // uart_sendline("order = %d, index = %d\n", page->order, page->idx);
        tmp = merge_buddy(page);
    }
    uart_sendline("After merging\n");
    uart_sendline("order = %d, index = %d\n", page->order, page->idx);
    //*page = page_array[tmp];
    page->used = PAGE_FREE;
    // uart_sendline("%d\n", page->idx);
    list_add(&page->listhead, &free_page_list[page->order]);

    // : print
    uart_sendline("After free pages\n");
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
    unsigned int b_start_idx = page->idx;   //first page index
    unsigned int b_end_idx = b_start_idx + pow(2, page->order) - 1; //last page index

    page->order -= 1;
    page_t *buddy = find_buddy(page);
    buddy->order = page->order;
    list_add(&buddy->listhead, &free_page_list[buddy->order]);
    uart_sendline("\tAfter spliting\n");
    print_page_info();

    unsigned int af_start_idx = page->idx;  //first page index
    unsigned int af_end_idx = af_start_idx + pow(2, page->order) - 1;   //last pafe index
    unsigned int as_start_idx = buddy->idx; // buddy page index
    unsigned int as_end_idx = as_start_idx + pow(2, buddy->order) - 1;  //buddy last pafe index

    uart_sendline("\tSplit from %d ~ %d to %d ~ %d and %d to %d\n\n", b_start_idx, b_end_idx, af_start_idx, af_end_idx, as_start_idx, as_end_idx);
    uart_sendline("\t=========================\n");
    return page;
}

void swap(int*p1 ,int *p2){
	int temp;
	temp=*p1;
	*p1=*p2 ;
	*p2=temp ;
}

int merge_buddy(page_t *page)
{
    int return_value = 0;
    page_t *buddy = find_buddy(page);
    uart_sendline("\nMerging...\nBefore merging\n");
    uart_sendline("order = %d, index = %d, used = %s, buddy order = %d, buddy index = %d, buddy used = %s\n", page->order, page->idx, page->used==PAGE_FREE?"Free":"Allocated", buddy->order, buddy->idx, buddy->used==PAGE_FREE?"Free":"Allocated");

    // page的order已經最大, page order跟buddy order不同, buddy正在被使用 => 不可merge
    if (page->order == PAGE_MAX_IDX || page->order != buddy->order || buddy->used == PAGE_ALLOCATED){
        uart_sendline("Merge error: can't merge pages\n");
        return INVALID_ADDRESS;
    }
    
    if (page->idx < buddy->idx)
    {
    }
    else if (page->idx > buddy->idx)
    {
        // uart_sendline("page = %d, buddy = %d\n", page->idx, buddy->idx);
        swap(&page ,&buddy);
        // uart_sendline("page = %d, buddy = %d\n", page->idx, buddy->idx);
        return_value = 1;
    }
    else
    {
        uart_sendline("Merge error: can't merge pages!\n");
        return INVALID_ADDRESS;
    }
    //uart_sendline("swaped\n");
    list_del_entry((list_head_t *)buddy);
    page->order += 1;

    uart_sendline("After merging\n");
    uart_sendline("order = %d, index = %d\n", page->order, page->idx);

    return return_value;
}

void get_new_slab(int order)
{
    uart_sendline("Get new slabs from a page\n");
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

    uart_sendline("Partition idx = %d page (0x%x) into slabs\n", page_ptr->idx, (unsigned long long)page);
}

// 一開始全部都是空的，如有需要就從page切成同等大小的slab存入該order的slab list
void *slab_malloc(unsigned int size)
{
    // : print
    uart_sendline("Before allocate new slabs\n");
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
    uart_sendline("After allocate new slabs\n");
    print_slab_info();
    return slab;
}

void slab_free(void *ptr)
{
    list_head_t *tmp = (list_head_t *)ptr;
    // 透過address取的切成此大小slab的page number
    page_t *page = &page_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12];

    // : print
    uart_sendline("Before free slabs\n");
    print_slab_info();
    list_add(tmp, &slab_list[page->slab_order]);

    // : print
    uart_sendline("After free slabs\n");
    print_slab_info();
}

void *kmalloc(unsigned int size)
{
    uart_sendline("\nRequest for size: %d\n", size);
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
        uart_sendline("Return avalible address: 0x%x\n\n", address);
    return address;
}

void kfree(void *ptr)
{
    uart_sendline("\nFree address: 0x%x\n", ptr);
    if((unsigned long long)ptr == INVALID_ADDRESS){
        uart_sendline("Free error: It's not a valid address.\n");
        return;
    }
    // using pages
    if (page_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12].slab_order == SLAB_NONE)
    {
        page_free(ptr);
        return;
    }
    slab_free(ptr);
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
    uart_sendline("Before reserving...\n");
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
                uart_sendline("\t[%d]Reserving 0x%x (%d) ~ 0x%x (%d)\n", order, start_addr, start_page_idx, end_addr, end_page_idx);
                uart_sendline("\tBefore reserving...\n");
                print_page_info();

                list_del_entry(cur);

                uart_sendline("\tAfter reserving...\n");
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
                uart_sendline("\n\tSpliting...\n\tBefore spliting\n");
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
    uart_sendline("\nAfter reserving...\n");
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
    uart_sendline("\t[Pages info]: \n\t");
    for (int i = 0; i <= PAGE_MAX_IDX; i++)
    {
        uart_sendline("%dKB (Order: %d): %d | ", 4 * pow(2, i), i, list_size(&free_page_list[i]));
    }
    uart_sendline("\n\t===============================\n");
}

void print_slab_info()
{
    uart_sendline("\t[Slabs info]: \n\t");
    for (int i = 0; i <= SLAB_MAX_IDX; i++)
    {
        uart_sendline("%dB (Order: %d): %d | ", 16 * pow(2, i), i, list_size(&slab_list[i]));
    }
    uart_sendline("\n\t===============================\n");
}