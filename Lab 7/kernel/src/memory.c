#include "memory.h"
#include "chunk.h"
#include "frame.h"
#include "dtb.h"
#include "initrd.h"


byteptr_t
memory_align(const byteptr_t p, uint32_t s) {
	uint64_t x = (uint64_t) p;
    s = (1 << s);
	return (byteptr_t) (x & ~(s - 1));
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


void
memory_copy(byteptr_t dest, byteptr_t source, int32_t size)
{
    while (size > 0) { *dest++ = *source++; --size; }
}


void
memory_zero(byteptr_t dest, int32_t size)
{
    while (size > 0) { *dest++ = 0; --size; }
}


#define STARTUP_HEAP_START      0x120000
#define STARTUP_HEAP_END        0x180000

extern byte_t kernel_end;

static byteptr_t _smalloc_cur = (byteptr_t) (STARTUP_HEAP_START | 0l);
static byteptr_t _smalloc_limit = (byteptr_t) (STARTUP_HEAP_END | 0l);



byteptr_t
startup_memory_alloc(uint32_t size)
{
    size = (uint32_t) memory_padding((byteptr_t) (size | 0l), 3);
    if ((uint32_t) (_smalloc_cur + size) > (uint32_t) _smalloc_limit) return 0;
    byteptr_t ret_ptr = _smalloc_cur;
    _smalloc_cur += size;
    return ret_ptr;
}


byteptr_t 
memory_alloc(uint32_t size)
{
    size = (uint32_t) memory_padding((byteptr_t) (size | 0l), 3);
    if ((uint32_t) (_smalloc_cur + size) > (uint32_t) _smalloc_limit) return 0;
    byteptr_t ret_ptr = _smalloc_cur;
    _smalloc_cur += size;
    return ret_ptr;
}


extern byte_t kernel_start, kernel_end;

void
memory_system_init(uint32_t start, uint32_t end)
{
    frame_system_init_frame_array(start, end);

    frame_system_preserve(0, 0x1000);                                                   // spin table
    frame_system_preserve(CHUNK_TABLE_ADDR, CHUNK_TABLE_END);
    frame_system_preserve(0x60000, 0x80000);                                            // call stack for kernel
    frame_system_preserve((uint32_t) &kernel_start, (uint32_t) &kernel_end + 0x4000);   // preserve some space for sprintf
    frame_system_preserve(STARTUP_HEAP_START, STARTUP_HEAP_END);
    frame_system_preserve((uint32_t) initrd_get_ptr(), (uint32_t) initrd_get_end());
    frame_system_preserve((uint32_t) dtb_get_ptr(), (uint32_t) dtb_get_end());

    frame_system_init_free_blocks();
    init_chunk_table();
}

#define BLOCK_HEADER_SIZE   8

typedef struct block_header {
    uint32_t count;
    uint32_t size;
} block_header_t;

typedef block_header_t * block_header_ptr;

#include "uart.h"

byteptr_t
kmemory_alloc(uint32_t size)
{
    // uart_printf("[DEBUG] kmemory_alloc - size: %d, %d\n", size, size + BLOCK_HEADER_SIZE);

    if (size <= chunk_max_size())
        return chunk_request(size);
    
    int32_t count = UINT32_PADDING((size + BLOCK_HEADER_SIZE), FRAME_SIZE_ORDER) >> FRAME_SIZE_ORDER;
    
    uart_printf("[DEBUG] kmemory_alloc - size: 0x%x, count: %d\n", size + BLOCK_HEADER_SIZE, count);

    byteptr_t block = frame_alloc(count);
    
    ((block_header_ptr) block)->count = count;
    ((block_header_ptr) block)->size = size + BLOCK_HEADER_SIZE;

    return block + BLOCK_HEADER_SIZE;
}


void
kmemory_release(byteptr_t ptr)
{
    // uart_printf("[DEBUG] kmemory_release - ptr: 0x%x\n", ptr);

    if ((uint32_t) ptr & chunk_data_offset()) {
        chunk_release(ptr);
        return;
    }
    
    ptr = ptr - BLOCK_HEADER_SIZE;
    uint32_t count = ((block_header_ptr) ptr)->count;
    for (uint32_t i = 0; i < count; ++i) {
        frame_release(ptr);
        ptr = (byteptr_t) ((uint64_t) ptr + (1 << FRAME_SIZE_ORDER));
    }
}


void
memory_print_info()
{
    frame_print_info();
    chunk_system_infro();
}