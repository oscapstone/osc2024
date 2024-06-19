#include "malloc.h"
#include "cpio.h"
#include "mmu.h"
extern char _heap_start;
extern char _end;
static char *simple_top = &_heap_start;
static char *kernel_end = &_end;
extern char *dtb_base;

// simple_malloc
void *smalloc(unsigned int size)
{
    char *r = simple_top + 0x10;
    if (size < 0x18)
        size = 0x18; // minimum size 0x20 //like ptmalloc
    size = size + 0x7;
    size = 0x10 + size - size % 0x10;
    *(unsigned int *)(r - 0x8) = size;
    simple_top += size;
    return r;
}

//buddy system allocator
//https://oscapstone.github.io/labs/lab4.
// use val(size) and isused with freelist is enough
//page size = 4K
/* For val (-2 ~ 6)
>=0 -> There is an allocable, contiguous memory that starts from the idx’th frame with size = 2**val * 4kb.
-1  -> allocated (deprecated)
-2  -> free, but it belongs to a larger contiguous memory block (deprecated)
*/

static frame_t* framearray;
static list_head_t freelist[MAXORDER + 1];       // 4K * (idx**ORDER) (for every 4K) (page)
static list_head_t cachelist[MAXCACHEORDER + 1]; // 32, 64, 128, 256, 512  (for every 32bytes)

void init_allocator()
{
    // The usable memory region is from 0x00 to 0x3C000000, you can get this information from the memory node in devicetree.
    // Advanced Exercise 3 - Startup Allocation - 20%
    framearray = smalloc(BUDDYSYSTEM_PAGE_COUNT * sizeof(frame_t));

    // init framearray
    for (int i = 0; i < BUDDYSYSTEM_PAGE_COUNT; i++)
    {
        if (i % (1 << MAXORDER) == 0)
        {
            framearray[i].isused = 0;
            framearray[i].val = 6;
        }
    }

    //init frame freelist
    for (int i = 0; i <= MAXORDER; i++)
    {
        INIT_LIST_HEAD(&freelist[i]);
    }

    for (int i = 0; i < BUDDYSYSTEM_PAGE_COUNT; i++)
    {
        //init listhead for each frame
        INIT_LIST_HEAD(&framearray[i].listhead);
        framearray[i].idx = i;
        framearray[i].cacheorder = -1;

        //add init frame into freelist
        if (i % (1 << MAXORDER) == 0)
        {
            list_add(&framearray[i].listhead, &freelist[MAXORDER]);
        }
    }

    //init cachelist
    for (int i = 0; i <= MAXCACHEORDER; i++)
    {
        INIT_LIST_HEAD(&cachelist[i]);
    }


    /* should reserve these memory region
    Spin tables for multicore boot (0x0000 - 0x1000)
    Kernel image in the physical memory
    Initramfs
    Devicetree (Optional, if you have implement it)
    Your simple allocator (startup allocator)  
    stack
   */
    memory_reserve(PHYS_TO_VIRT(0x0000), PHYS_TO_VIRT(0x1000)); 
    memory_reserve(PHYS_TO_VIRT(0x1000), PHYS_TO_VIRT(0x5000)); // // PGD's page frame at 0x1000 // PUD's page frame at 0x2000 PMD 0x3000-0x5000
    memory_reserve(PHYS_TO_VIRT(0x80000), (unsigned long long)kernel_end);              //kernel
    memory_reserve((unsigned long long)&_heap_start, (unsigned long long)simple_top); //simple
    memory_reserve((unsigned long long)cpio_start, (unsigned long long)cpio_end);
    memory_reserve(PHYS_TO_VIRT(0x2c000000), PHYS_TO_VIRT(0x3c000000)); //0x2c000000L - 0x3c000000L (stack)
}

//smallest 4K
void *allocpage(unsigned int size)
{
    // get real val size
    //int allocsize;
    int val;
    for (int i = 0; i <= MAXORDER; i++)
    {

        if (size <= (0x1000 << i))
        {
            val = i;
            break;
        }

        if (i == MAXORDER)
        {
            uart_puts("Too large size for malloc!!!!\r\n");
            while (1);
            return smalloc(size);
        }
    }

    // find the smallest larger frame in freelist
    int target_list_val;
    for (target_list_val = val; target_list_val <= MAXORDER; target_list_val++)
    {
        if (!list_empty(&freelist[target_list_val]))
            break;
    }

    if (target_list_val > MAXORDER)
    {
        uart_puts("malloc ERROR (all lists are empty?)!!!!\r\n");
        while (1);
        return smalloc(size);
    }

    //get the frame
    frame_t *target_frame_ptr = (frame_t *)freelist[target_list_val].next;
    list_del((struct list_head *)target_frame_ptr);

    // Release redundant memory block
    for (int j = target_list_val; j > val; j--)
    {
        release_redundant(target_frame_ptr);
    }
    target_frame_ptr->isused = 1;
#ifdef DEBUG
    uart_printf("allocpage ret : 0x%x, val : %d\r\n", BUDDYSYSTEM_START + (0x1000 * (target_frame_ptr->idx)), target_frame_ptr->val);
#endif
    return (void *)BUDDYSYSTEM_START + (0x1000 * (target_frame_ptr->idx));
}

void freepage(void *ptr)
{
    frame_t *target_frame_ptr = &framearray[((unsigned long long)ptr - BUDDYSYSTEM_START) >> 12];

#ifdef DEBUG
    uart_printf("freepage 0x%x, val = %d\r\n", ptr, target_frame_ptr->val);
#endif

    target_frame_ptr->isused = 0;
    frame_t* temp;
    while ((temp = coalesce(target_frame_ptr)) != (frame_t *)-1)target_frame_ptr = temp;

    list_add(&target_frame_ptr->listhead, &freelist[target_frame_ptr->val]);
}

frame_t *release_redundant(frame_t *frame_ptr)
{
    frame_ptr->val -= 1;
    frame_t *buddyptr = get_buddy(frame_ptr);
    buddyptr->val = frame_ptr->val;
    buddyptr->isused = 0;
    list_add(&buddyptr->listhead, &freelist[buddyptr->val]);
#ifdef DEBUG
    uart_printf("release_redundant idx = %d,%d\r\n", frame_ptr->idx, buddyptr->idx);
#endif
    return frame_ptr;
}

frame_t *get_buddy(frame_t *frame)
{
    return &framearray[frame->idx ^ (1 << frame->val)];
}

//return 0  -> success
//return -1 -> cannot coalesce
frame_t* coalesce(frame_t *frame_ptr)
{
    frame_t *buddy = get_buddy(frame_ptr);

    // MAXORDER
    if (frame_ptr->val == MAXORDER)
        return (frame_t*)-1;

    // val not the same (there is some chunks in the buddy used)
    if (frame_ptr->val != buddy->val)
        return (frame_t *)-1;

    //buddy is used
    if (buddy->isused == 1)
        return (frame_t *)-1;

    list_del((struct list_head *)buddy);
    frame_ptr->val += 1;
    buddy->val += 1;

#ifdef DEBUG
        uart_printf("coalesce idx = %d,%d\r\n", frame_ptr->idx, buddy->idx);
#endif
    return buddy<frame_ptr?buddy:frame_ptr;
}

void *alloccache(unsigned int size)
{
    //get order
    int order;
    for (int i = 0; i <= MAXCACHEORDER; i++)
    {
        if (size <= (32 << i))
        {
            order = i;
            break;
        }
    }

    if (list_empty(&cachelist[order]))
    {
        page2caches(order);
    }

    list_head_t *r = cachelist[order].next;
    list_del(r);

#ifdef DEBUG
    uart_printf("alloc cache order : %d\r\n", order);
#endif
    return r;
}

void page2caches(int order)
{
    //make caches of the order from a page
    char *page = allocpage(0x1000);
    frame_t *pageframe_ptr = &framearray[((unsigned long long)page - BUDDYSYSTEM_START) >> 12];
    pageframe_ptr->cacheorder = order;

    // split page into a lot of caches and push them into cachelist
    int cachesize = (32 << order);
    for (int i = 0; i < 0x1000; i += cachesize)
    {
        list_head_t *c = (list_head_t *)(page + i);
        list_add(c, &cachelist[order]);
    }
}

void freecache(void *ptr)
{
    list_head_t *c = (list_head_t *)ptr;
    frame_t *pageframe_ptr = &framearray[((unsigned long long)ptr - BUDDYSYSTEM_START) >> 12];
    list_add(c, &cachelist[pageframe_ptr->cacheorder]);
#ifdef DEBUG
    uart_printf("freecache 0x%x, order : %d\r\n", ptr, pageframe_ptr->cacheorder);
#endif
}
// malloc -> virtual -> 0xffff...
// because of linker script
void *malloc(unsigned int size)
{
    lock();
#ifdef DEBUG
    uart_printf("malloc size: %d\r\n", size);
#endif
    //For page
    if (size > (32 << MAXCACHEORDER))
    {
        void *r = allocpage(size);
#ifdef DEBUG
        dump_freelist_info();
        dump_cachelist_info();
#endif
        unlock();
        return r;
    }

    void *r = alloccache(size);

#ifdef DEBUG
    dump_freelist_info();
    dump_cachelist_info();
    uart_printf("malloc ret 0x%x\r\n", r);
#endif
    //For cache
    unlock();
    return r;
}

void free(void *ptr)
{
    lock();
#ifdef DEBUG
    uart_printf("free 0x%x\r\n", ptr);
#endif
    //For page
    if ((unsigned long long)ptr % 0x1000 == 0 && framearray[((unsigned long long)ptr - BUDDYSYSTEM_START) >> 12].cacheorder == -1)
    {
        freepage(ptr);
#ifdef DEBUG
        dump_freelist_info();
        dump_cachelist_info();
#endif
        unlock();
        return;
    }

    //For cache
    freecache(ptr);
#ifdef DEBUG
    dump_freelist_info();
    dump_cachelist_info();
#endif
    unlock();
}

void dump_freelist_info()
{
    for (int i = 0; i <= MAXORDER; i++)
        uart_printf("freelist %d : %d\r\n", i, list_size(&freelist[i]));
}

void dump_cachelist_info()
{
    for (int i = 0; i <= MAXCACHEORDER; i++)
        uart_printf("cachelist %d : %d\r\n", i, list_size(&cachelist[i]));
}

void memory_reserve(unsigned long long start, unsigned long long end)
{
    uart_printf("reserve -> start : 0x%x end : 0x%x\r\n", start, end);
    start -= start % 0x1000; // floor (align 0x1000)
    end = end % 0x1000 ? end + 0x1000 - (end % 0x1000) : end; // ceiling (align 0x1000)

    //delete page from freelist
    for (int order = MAXORDER; order >= 0; order--)
    {
        list_head_t *pos;
        list_for_each(pos, &freelist[order])
        {
            unsigned long long pagestart = ((frame_t *)pos)->idx * 0x1000L + BUDDYSYSTEM_START;
            unsigned long long pageend = pagestart + (0x1000L << order);

            if (start <= pagestart && end >= pageend) // if page all in reserved memory -> delete it from freelist
            {
                ((frame_t *)pos)->isused = 1;
                list_del(pos);
#ifdef DEBUG
                uart_printf("del order %d\r\n",order);
                //dump_freelist_info();
#endif
            }
            else if (start >= pageend || end <= pagestart) // no intersection
            {
                continue;
            }
            else // partial intersection (or reversed memory all in the page)
            {
                list_del(pos);
                list_head_t *temppos = pos -> prev;
                list_add(&release_redundant((frame_t *)pos)->listhead, &freelist[order - 1]);
                pos = temppos;
#ifdef DEBUG
                //dump_freelist_info();
#endif
            }
        }
    }
}

void alloctest()
{
    uart_printf("alloc test\r\n");

    //memory_reserve(0x1FFFAddb, 0x1FFFFdda);
    
    char *a = malloc(0x10);
    char *b = malloc(0x100);
    char *c = malloc(0x1000);

    free(a);
    free(b);
    free(c);

    a = malloc(32);
    char *aa = malloc(50);
    b = malloc(64);
    char *bb = malloc(64);
    c = malloc(128);
    char *cc = malloc(129);
    char *d = malloc(256);
    char *dd = malloc(256);
    char *e = malloc(512);
    char *ee = malloc(999);

    free(a);
    free(aa);
    free(b);
    free(bb);
    free(c);
    free(cc);
    free(dd);
    free(d);
    free(e);
    free(ee);
}

void *memcpy(void *dest, const void *src, unsigned long long n) {
    char *cdest = dest;
    const char *csrc = src;

    while (n--)
    {
        *cdest++ = *csrc++;
    }

    return dest;
}