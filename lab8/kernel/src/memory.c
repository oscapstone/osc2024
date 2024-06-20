#include "memory.h"
#include "u_list.h"
#include "uart1.h"
#include "exception.h"
#include "dtb.h"
#include "stdio.h"
#include "mmu.h"
#include "bcm2837/rpi_mmu.h"

extern char _heap_top;
static char *htop_ptr = &_heap_top;

extern char _kernel_start;
extern char _kernel_end;
extern char *CPIO_DEFAULT_START;
extern char *CPIO_DEFAULT_END;
extern char  _stack_end;
extern char  _stack_top;

void *allocator(unsigned int size)
{
    lock();
    // -> htop_ptr
    // htop_ptr + 0x02:  heap_block size
    // htop_ptr + 0x10 ~ htop_ptr + 0x10 * k:
    //            { heap_block }
    // -> htop_ptr
    //               header 0x10 bytes                   block
    // |--------------------------------------------------------------|
    // |  fill zero 0x8 bytes | size 0x8 bytes | size padding to 0x16 |
    // |--------------------------------------------------------------|

    // 0x10 for heap_block header
    char *r = htop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned int *)(r - 0x8) = size;
    htop_ptr += size;
    unlock();
    return r;
}

// void free(void* ptr) {
//     // TBD
// }

// ------------------------------------------------------------

static frame_t *frame_array;                      // store memory's statement and page's corresponding index
static list_head_t frame_freelist[FRAME_MAX_IDX]; // store available block for page
static list_head_t cache_list[CACHE_MAX_IDX];     // store available block for cache

void allocator_init()
{
    frame_array = allocator(BUDDY_MEMORY_PAGE_COUNT * sizeof(frame_t));

    // init frame freelist
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
    {
        INIT_LIST_HEAD(&frame_freelist[i]);
    }

    // init cache list
    for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++)
    {
        INIT_LIST_HEAD(&cache_list[i]);
    }

    for (int i = 0; i < BUDDY_MEMORY_PAGE_COUNT; i++)
    {
        // init listhead for each frame
        INIT_LIST_HEAD(&(frame_array[i].listhead));
        frame_array[i].idx = i;
    }
    freelist_init();
    dump_page_info();

    /* Startup reserving the following region:
    Spin tables for multicore boot (0x0000 - 0x1000)
    Devicetree (Optional, if you have implement it)
    Kernel image in the physical memory
    Your simple allocator (startup allocator) (Stack + Heap in my case)
    Initramfs
    */
    uart_sendlinek("\r\n* Startup Allocation *\r\n");
    uart_sendlinek("buddy system: usable memory region: 0x%x ~ 0x%x\n", BUDDY_MEMORY_BASE, BUDDY_MEMORY_BASE + BUDDY_MEMORY_PAGE_COUNT * PAGESIZE);
    dtb_find_and_store_reserved_memory();                                                 // find spin tables in dtb
    memory_reserve(PHYS_TO_KERNEL_VIRT(MMU_PGD_ADDR), PHYS_TO_KERNEL_VIRT(MMU_PTE_ADDR + 0x2000));      // // PGD's page frame at 0x1000 // PUD's page frame at 0x2000 PMD 0x3000-0x5000
    memory_reserve((unsigned long long)&_kernel_start, (unsigned long long)&_kernel_end); // kernel
    memory_reserve((unsigned long long)&_stack_end, (unsigned long long)&_stack_top);
    memory_reserve((unsigned long long)CPIO_DEFAULT_START, (unsigned long long)CPIO_DEFAULT_END);
}

void freelist_init()
{
    int PAGE_COUNT = BUDDY_MEMORY_PAGE_COUNT;
    for (int i = 0; i <= FRAME_IDX_FINAL; i++)
    {
        // uart_sendlinek("\n %ld,%ld \n",PAGE_COUNT,(1 << i));
        if ((PAGE_COUNT & (1 << i)))
        {
            PAGE_COUNT -= (1 << i);
            frame_array[PAGE_COUNT].val = i;
            frame_array[PAGE_COUNT].used = FRAME_FREE;
            list_add(&(frame_array[PAGE_COUNT].listhead), &frame_freelist[i]);
        }
        if (i == FRAME_IDX_FINAL && PAGE_COUNT)
        {
            while (PAGE_COUNT)
            {
                PAGE_COUNT -= (1 << i);
                frame_array[PAGE_COUNT].val = FRAME_IDX_FINAL;
                frame_array[PAGE_COUNT].used = FRAME_FREE;
                list_add(&(frame_array[PAGE_COUNT].listhead), &frame_freelist[FRAME_IDX_FINAL]);
            }
        }
    }
}

frame_t *release_redundant(frame_t *frame)
{
    // order -1 -> add its buddy to free list (frame itself will be used in master function)
    frame->val -= 1;
    frame_t *buddyptr = get_buddy(frame);
    buddyptr->val = frame->val;
    buddyptr->used = FRAME_FREE;
    list_add(&(buddyptr->listhead), &frame_freelist[buddyptr->val]);
    return frame;
}

frame_t *get_buddy(frame_t *frame)
{
    // XOR(idx, order)
    if ((frame->idx ^ (1 << frame->val)) > BUDDY_MEMORY_PAGE_COUNT)
    {
        uart_sendlinek("[!] BUDDY of Page: 0x%x at level: %d Does not exit", frame->idx, frame->val);
        // return -1;
    }
    return &frame_array[frame->idx ^ (1 << frame->val)];
}

void dump_page_info()
{
    // unsigned int exp2 = 1;
    // uart_sendlinek("        ┌───────────────────── [  Number of Available Page Blocks  ] ─────────────────────┐\r\n        │ ");
    // for (int i = FRAME_IDX_0; i < FRAME_IDX_8; i++)
    // {
    //     uart_sendlinek("%4dKB(%1d) ", 4 * exp2, i);
    //     exp2 *= 2;
    // }
    // uart_sendlinek("│\r\n        │ ");
    // for (int i = FRAME_IDX_0; i < FRAME_IDX_8; i++)
    //     uart_sendlinek(" %4d     ", list_size(&frame_freelist[i]));
    // uart_sendlinek("│\r\n        ");
    // uart_sendlinek("└────────────────────────────────────────");
    // uart_sendlinek("─────────────────────────────────────────┘\r\n");

    // exp2 = 1;
    // uart_sendlinek("        ┌──────────────────────────────────── ");
    // uart_sendlinek("[  Number of Available Page Blocks  ]");
    // uart_sendlinek(" ───────────────────────────────────┐\r\n        │");
    // for (int i = FRAME_IDX_8; i < FRAME_MAX_IDX; i++)
    // {
    //     uart_sendlinek("%4dMB(%2d) ", exp2, i);
    //     exp2 *= 2;
    // }
    // uart_sendlinek("│\r\n        │");
    // for (int i = FRAME_IDX_8; i < FRAME_MAX_IDX; i++)
    //     uart_sendlinek("  %4d     ", list_size(&frame_freelist[i]));
    // uart_sendlinek("│\r\n        ");
    // uart_sendlinek("└──────────────────────────────────────────────────────");
    // uart_sendlinek("────────────────────────────────────────────────────────┘\r\n");
}

void dump_cache_info()
{
    // unsigned int exp2 = 1;
    // uart_sendlinek("    ┌──────────────── [  Number of Available Cache Blocks ] ────────────────┐\r\n    │ ");
    // for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++)
    // {
    //     uart_sendlinek("%4dB(%1d)  ", CACHE_SEG * exp2, i);
    //     exp2 *= 2;
    // }
    // uart_sendlinek("│\r\n    │ ");
    // ;
    // for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++)
    // {
    //     if (!list_empty(&cache_list[i]))
    //     {
    //         // uart_sendlinek("I free !!!!!!!!!!!!");
    //         int num = 0;
    //         list_head_t *pos;
    //         list_for_each(pos, &cache_list[i])
    //         {
    //             num += ((cache_t *)pos)->available;
    //         }
    //         uart_sendlinek("%5d     ", num);
    //     }
    //     else
    //     {
    //         uart_sendlinek("%5d     ", 0);
    //     }
    // }

    // uart_sendlinek("│\r\n    ");
    // uart_sendlinek("└───────────────────────────────────");
    // uart_sendlinek("────────────────────────────────────┘\r\n");
}

void memory_reserve(unsigned long long start, unsigned long long end)
{
    start -= start % PAGESIZE;                                      // floor (align 0x1000)
    end = end % PAGESIZE ? end + PAGESIZE - (end % PAGESIZE) : end; // ceiling (align 0x1000)

    // uart_sendlinek("Reserved Memory: ");
    // uart_sendlinek("start 0x%x ~ ", start);
    // uart_sendlinek("end 0x%x\r\n", end);

    // delete page from free list
    for (int order = FRAME_IDX_FINAL; order >= 0; order--)
    {
        list_head_t *pos;
        // uart_sendlinek("\n use %d level page to match \n",order);
        list_for_each(pos, &frame_freelist[order])
        {
            unsigned long long pagestart = ((frame_t *)pos)->idx * PAGESIZE + BUDDY_MEMORY_BASE;
            unsigned long long pageend = pagestart + (PAGESIZE << order);
            // uart_sendlinek("\n from 0x%x to 0x%x\n",pagestart,pageend);

            if (start <= pagestart && end >= pageend) // if page all in reserved memory -> delete it from freelist
            {
                ((frame_t *)pos)->used = FRAME_ALLOCATED;
                // uart_sendlinek("    [!] Reserved page in 0x%x - 0x%x\n", pagestart, pageend);
                // uart_sendlinek("        Before\n");
                // dump_page_info();
                list_del_entry(pos);
                // uart_sendlinek("        Remove usable block for reserved memory: order %d\r\n", order);
                // uart_sendlinek("        After\n");
                // dump_page_info();
            }
            else if (start >= pageend || end <= pagestart) // no intersection
            {
                continue;
            }
            else // partial intersection, separate the page into smaller size.
            {
                // dump_page_info();
                list_del_entry(pos);
                list_head_t *temppos = pos->prev;
                list_add(&release_redundant((frame_t *)pos)->listhead, &frame_freelist[order - 1]);
                pos = temppos;
                // dump_page_info();
            }
        }
    }
}

void *kmalloc(unsigned int size)
{
    lock();
    
    // uart_sendlinek("\n\n");
    // uart_sendlinek("================================\r\n");
    // uart_sendlinek("[+] Request kmalloc size: %d\r\n", size);
    // uart_sendlinek("================================\r\n");
    // if size is larger than cache size, go for page
    if (size > (CACHE_SEG << CACHE_IDX_FINAL))
    {
        void *r = page_malloc(size);
        unlock();
        return r;
    }
    // go for cache
    void *r = cache_malloc(size);
    unlock();
    return r;
};

void *page_malloc(unsigned int size)
{
    // uart_sendlinek("this is page_malloc \r\n");
    int val;
    int find_PageSize = 0;
    frame_t *target_frame_ptr;
    // turn size into minimum 4KB * 2**val
    for (int i = FRAME_IDX_0; i <= FRAME_IDX_FINAL; i++)
    {

        if (size <= (PAGESIZE << i) && !find_PageSize)
        {
            val = i;
            // uart_sendlinek("        block size = 0x%x\n", PAGESIZE << i);
            find_PageSize = 1;
        }

        if (find_PageSize && !list_empty(&frame_freelist[i]))
        {
            // uart_sendlinek(" free page at level : %d\n", i);
            target_frame_ptr = (frame_t *)(frame_freelist[i].next);
            for (int min_FreePageLevel = i; min_FreePageLevel > val; min_FreePageLevel--) // ex: 10000 -> 01111
            {
                target_frame_ptr = release_redundant(target_frame_ptr);
            }
            break;
        }

        if (i == FRAME_IDX_FINAL)
        {
            // uart_sendlinek("[!] request size exceeded for page_malloc!!!!\r\n");
            return (void *)0;
        }
    }
    // get the available frame from freelist
    target_frame_ptr->used = FRAME_ALLOCATED;

    // uart_sendlinek("    [+] Allocate page - size : %d(0x%x)\r\n", size, size);
    // uart_sendlinek("        Before\r\n");
    // dump_page_info();
    list_del_entry((struct list_head *)target_frame_ptr);
    // uart_sendlinek("        physical address : 0x%x\n", BUDDY_MEMORY_BASE + (PAGESIZE * (target_frame_ptr->idx)));
    // uart_sendlinek("        After\r\n");
    // dump_page_info();

    return (void *)BUDDY_MEMORY_BASE + (PAGESIZE * (target_frame_ptr->idx));
};

void *cache_malloc(unsigned int size)
{
    int c_val;
    // uart_sendlinek("this is cache_malloc \r\n");
    for (int i = CACHE_IDX_0; i <= CACHE_IDX_FINAL; i++)
    {
        if (size <= (CACHE_SEG << i))
        {
            c_val = i;
            break;
        }
    }

    if (list_empty(&cache_list[c_val]))
    {
        // uart_sendlinek("[!] No free size for cache \r\n"); //---------------------------------------------------
        page2caches(c_val);
    }

    // cache_t *ptr = (cache_t *)(cache_list[c_val].prev);

    // uart_sendlinek("this is cache_malloc \r\n");
    // uart_sendlinek("[+] Allocate cache - size : %d(0x%x)\r\n", size, size);
    // uart_sendlinek("    Before\r\n");
    // dump_cache_info();

    void *ptr = find_CACHE((cache_t *)cache_list[c_val].next);
    // uart_sendlinek("ptr : %x\n",ptr);       
    // uart_sendlinek("    After\r\n");
    // dump_cache_info();

    return ptr;
};

void *find_CACHE(cache_t *ptr)
{
    // uart_sendlinek("ptr : %x\n",ptr);
    unsigned long long num;
    int record_num;
    for (record_num = 0; ptr->cache_record[record_num] + 1 == 0 && record_num < CACHE_record_num; record_num++)
        ;

    num = ptr->cache_record[record_num] + 1;
    num = ((~num) & (ptr->cache_record[record_num])) + 1;

    // uart_sendlinek("before      cache_record[%d] : %x\n", record_num, ptr->cache_record[record_num]);
    ptr->cache_record[record_num] += num;
    ptr->available--;
    // uart_sendlinek("after      cache_record[%d] : %x\n", record_num, ptr->cache_record[record_num]);

    int val = fake_log2(num) + record_num * 64;
    void *target = ptr->data_base + val * (CACHE_SEG << (ptr->cache_order));
    // uart_sendlinek("data_base addr : 0x%x\n", ptr->data_base);
    // uart_sendlinek("target addr : 0x%x\n", target);

    if (!(ptr->available))
    {
        list_del_entry((list_head_t *)ptr);
    }

    return target;
}

void page2caches(int c_val)
{
    // uart_sendlinek("[!] Split Page for cache\r\n");
    void *ptr = page_malloc(PAGESIZE);
    // uart_sendlinek("ptr : %x\n",ptr);
    cache_t *Pageinfo = (cache_t *)ptr;
    Pageinfo->data_base = ptr;
    Pageinfo->cache_order = c_val;
    Pageinfo->max_available = (PAGESIZE >> (CACHE_offset + c_val));

    while (sizeof(*Pageinfo) >= Pageinfo->data_base - ptr)
    {
        Pageinfo->data_base += (CACHE_SEG << c_val);
        (Pageinfo->max_available)--;
    }
    Pageinfo->available = Pageinfo->max_available;

    for (int i = 0; i < CACHE_record_num; i++)
        Pageinfo->cache_record[i] = 0;

    // uart_sendlinek("      max_available : %d\n", Pageinfo->max_available);
    // uart_sendlinek("      Pageinfo size : 0x%x\n", sizeof(*Pageinfo));
    // uart_sendlinek("      offset size :  0x%x\n", (CACHE_SEG << c_val));

    list_add(&(Pageinfo->listhead), &cache_list[c_val]);
    // return Pageinfo;
}

//------------------------------------------------------------------------------------------------------------------------------- kfree

void kfree(void *ptr)
{
    // uart_sendlinek("\r\n");
    // uart_sendlinek("==========================\r\n");
    //uart_sendlinek("[+] Request kfree 0x%x\r\n", ptr);
    // uart_sendlinek("==========================\r\n");

    // If no cache assigned, go for page
    lock();
    frame_t *target_frame_ptr = &frame_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12];
    if ((unsigned long long)ptr % PAGESIZE == 0)
    {
        page_free(target_frame_ptr);
        unlock();
        return;
    }
    // go for cache
    cache_free(ptr);
    unlock();
};

void page_free(frame_t *target_frame_ptr)
{
    frame_t *buddyptr = get_buddy(target_frame_ptr);
    target_frame_ptr->used = FRAME_FREE;
    while (buddyptr->used == FRAME_FREE && buddyptr->val == target_frame_ptr->val)
    {
        list_del_entry((list_head_t *)buddyptr);
        target_frame_ptr = &frame_array[(target_frame_ptr->idx) & (buddyptr->idx)];
        target_frame_ptr->val++;
        buddyptr = get_buddy(target_frame_ptr);
        if (buddyptr < 0)
            break;
    }

    // uart_sendlinek("        Before\r\n");
    // dump_page_info();
    list_add(&(target_frame_ptr->listhead), &frame_freelist[target_frame_ptr->val]);
    // uart_sendlinek("        After\r\n");
    // dump_page_info();
}

void cache_free(void *ptr)
{
    // uart_sendlinek("ptr : %x\n",ptr);
    int idx = ((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12;
    frame_t *target_frame_ptr = &frame_array[idx];
    cache_t *cache_ptr = (cache_t *)((unsigned long long)(idx * PAGESIZE) + BUDDY_MEMORY_BASE); //<-------
    // uart_sendlinek("target_frame_ptr : %x\n",target_frame_ptr);
    //uart_sendlinek("cache_ptr : %x\n",cache_ptr);
    int num = (ptr - cache_ptr->data_base) / (CACHE_SEG << cache_ptr->cache_order);
    //uart_sendlinek("num : %d\n",num);
    int record_num = (num >> 6);
    num = num - (record_num << 6);
    cache_ptr->cache_record[record_num] -= (1 << num);

    // uart_sendlinek("[+] Free cache: 0x%x, val = %d\r\n", ptr, cache_ptr->cache_order);
    // uart_sendlinek("    Before\r\n");
    // dump_cache_info();
    if (cache_ptr->available == 0)
    {
        list_add(&(cache_ptr->listhead), &cache_list[cache_ptr->cache_order]);
    }
    cache_ptr->available++;
    if (cache_ptr->available == cache_ptr->max_available)
    {
        list_del_entry((list_head_t *)cache_ptr);
        page_free(target_frame_ptr);
    }
    // uart_sendlinek("    After\r\n");
    // dump_cache_info();
}