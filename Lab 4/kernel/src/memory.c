#include "memory.h"
#include "frame.h"
#include "dtb.h"
#include "initrd.h"


byteptr_t
memory_align(const byteptr_t p, uint32_t s) {
	uint64_t x = (uint64_t) p;
    s = (1 << s);
	return (byteptr_t) ((x + s - 1) & (~(s - 1)));
}


byteptr_t
memory_padding(const byteptr_t p, uint32_t s) {
	uint64_t x = (uint64_t) p;
    s = (1 << s);
	return (byteptr_t) ((x + s - 1) & (~(s - 1)));
}


int32_t
memory_cmp(byteptr_t s1, byteptr_t s2, int32_t n)
{
    byteptr_t a = s1, b = s2;
    while (n-- > 0) { if (*a != *b) { return *a - *b; } a++; b++; }
    return 0;
}


#define HEAP_START             0x100000
#define HEAP_END               0x120000

static byteptr_t _malloc_cur = (byteptr_t) HEAP_START;


byteptr_t 
memory_alloc(uint32_t size)
{
    size = (uint32_t) memory_padding((byteptr_t) (size | 0l), 2);
    if ((uint32_t) (_malloc_cur + size) > HEAP_END) return 0;
    byteptr_t ret_ptr = _malloc_cur;
    _malloc_cur += size;
    return ret_ptr;
}


#define SIMPLE_HEAP_START      0x90000
#define SIMPLE_HEAP_END        0xF0000

extern byte_t kernel_end;

static byteptr_t _smalloc_cur = (byteptr_t) (SIMPLE_HEAP_START | 0l);
static byteptr_t _smalloc_limit = (byteptr_t) (SIMPLE_HEAP_END | 0l);



byteptr_t
simple_memory_alloc(uint32_t size)
{
    size = (uint32_t) memory_padding((byteptr_t) (size | 0l), 3);
    if ((uint32_t) (_smalloc_cur + size) > (uint32_t) _smalloc_limit) return 0;
    byteptr_t ret_ptr = _smalloc_cur;
    _smalloc_cur = _smalloc_cur + size;
    return ret_ptr;
}


byteptr_t
memory_alloc_ptr(byteptr_t ptr, uint32_t request)
{
    // frame_alloc_addr(ptr, request);
    // return (uint32_t) ptr + (request << FRAME_SIZE_ORDER); 
    return 0;
}


void
memory_reserve(byteptr_t start, byteptr_t end)
{
}


extern byte_t kernel_end;


void
memory_system_init(uint32_t start, uint32_t end)
{
    frame_system_init_frame_array(start, end);

    frame_system_preserve(0, 0x1000);
    frame_system_preserve(0x60000, 0x80000);
    frame_system_preserve(0x80000, (uint32_t) &kernel_end + 0x10000);
    frame_system_preserve(SIMPLE_HEAP_START, SIMPLE_HEAP_END);
    frame_system_preserve(HEAP_START, HEAP_END);
    frame_system_preserve((uint32_t) initrd_get_ptr(), (uint32_t) initrd_get_end());
    frame_system_preserve((uint32_t) dtb_get_ptr(), (uint32_t) dtb_get_end());

    frame_system_init_free_blocks();
}


