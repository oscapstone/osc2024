#include "chunk.h"
#include "frame.h"
#include "list.h"
#include "uart.h"


#define CHUNK_UNIT_ORDER    4
#define CHUNK_MAX_COUNT     0xEF    // 240 - 1
#define CHUNK_MIN_SIZE      (1 << CHUNK_UNIT_ORDER)
#define CHUNK_MAX_SIZE      (CHUNK_MAX_COUNT * CHUNK_MIN_SIZE)

#define CHUNK_LIST_OFFSET   0x10    // 16
#define CHUNK_DATA_OFFSET   0x100   // 256

#define CHUNK_SIZE_INDEX    0xF0    // 240
#define CHUNK_COUNT_INDEX   0xF1    // 241

#define ADDR_TO_PTR(addr)   ({(byteptr_t)(addr | 0l);})
#define PTR_TO_ADDR(ptr)    ({(uint32_t) ptr;})

#define FRAME_ADDR(ptr)     ({(uint32_t) ptr & ~((1 << FRAME_SIZE_ORDER) - 1);})
#define CHUNK_LIST(ptr)     ({(FRAME_ADDR(ptr) + CHUNK_LIST_OFFSET);})
#define CHUNK_DATA(ptr)     ({(FRAME_ADDR(ptr) + CHUNK_DATA_OFFSET);})

#define CHUNK_TO_INDEX(ptr)     ({((uint32_t) ptr - CHUNK_DATA(ptr)) >> CHUNK_UNIT_ORDER;})
#define CLAMP_CHUNK_SIZE(size)  ({(size < CHUNK_MIN_SIZE) ? CHUNK_MIN_SIZE : (size > CHUNK_MAX_SIZE) ? CHUNK_MAX_SIZE : size;})


/**
 * chunk size: 1~15 (x16bytes)
*/
static void
init_frame(byteptr_t frame, uint32_t chunk_size)
{
    INIT_LIST_HEAD((list_head_ptr_t) frame);

    byteptr_t list = ADDR_TO_PTR(CHUNK_LIST(frame));
    
    list[0] = 1;
    for (int32_t i = 1; i < CHUNK_SIZE_INDEX; ++i) { list[i] = 0; }
    int32_t i = 1;
    while (i < CHUNK_SIZE_INDEX) { list[i] = i + chunk_size; i += chunk_size; }
    list[i - chunk_size] = 0;
    list[CHUNK_COUNT_INDEX] = 0;
    list[CHUNK_SIZE_INDEX] = (uint8_t) chunk_size;
}


static byteptr_t 
frame_request_chunk(byteptr_t frame)
{
    byteptr_t list = ADDR_TO_PTR(CHUNK_LIST(frame));

    // check top, it's full when top is zero
    if (list[0] == 0) return 0;

    // pop free list
    uint8_t top = list[0];
    uint8_t second = list[top];
    list[0] = second;

    // accumlate the counter (# of allocated 16-byte units)
    uint8_t size = list[CHUNK_SIZE_INDEX];
    list[CHUNK_COUNT_INDEX] += size;

    // index to ptr
    byteptr_t data = ADDR_TO_PTR(CHUNK_DATA(frame));
    uart_printf("[DEBUG] frame: 0x%x, data: 0x%x, top: %d\n", frame, data, top);
    return &data[top * (1 << CHUNK_UNIT_ORDER)];
}


/**
 * return value: 
 *  1 - this frame is empty after released
 *  0 - otherwise
*/
static uint32_t
frame_release_chunk(byteptr_t chunk)
{
    byteptr_t list = ADDR_TO_PTR(CHUNK_LIST(chunk));

    // push free list
    uint32_t index = CHUNK_TO_INDEX(chunk);
    list[index] = list[0];
    list[0] = index;

    // decrease the counter
    uint8_t size = list[CHUNK_SIZE_INDEX];
    list[CHUNK_COUNT_INDEX] -= size;

    // return 1 if empty
    return (list[CHUNK_COUNT_INDEX] == 0) ? 1 : 0;
}


static uint32_t
frame_count_free_chunks(byteptr_t frame)
{
    byteptr_t list = ADDR_TO_PTR(CHUNK_LIST(frame));
    uint32_t i = 0, count = 0;
    while (i < CHUNK_MAX_COUNT && list[i] != 0) { ++count; i = list[i]; }
    return count;
}


static list_head_ptr_t          chunk_table;

#define CHUNK_TABLE(i)          ({(list_head_ptr_t) &chunk_table[i];})
#define FRAME_IS_FULL(ptr)      ({ADDR_TO_PTR(CHUNK_LIST(ptr))[0] == 0;})
#define FRAME_IS_EMPTY(ptr)     ({ADDR_TO_PTR(CHUNK_LIST(ptr))[CHUNK_COUNT_INDEX] == 0;})


void
init_chunk_table()
{
    chunk_table = (list_head_ptr_t) CHUNK_TABLE_ADDR;
    for (uint32_t i = 0; i < CHUNK_MAX_COUNT; ++i)
        INIT_LIST_HEAD(CHUNK_TABLE(i));
}


void
chunk_release(byteptr_t ptr)
{
    frame_release_chunk(ptr);
    if (FRAME_IS_EMPTY(ptr)) {
        list_del_entry((list_head_ptr_t) ADDR_TO_PTR(FRAME_ADDR(ptr)));
        frame_release(ptr);
    }
}

/**
 * size: request momory size in bytes 
*/
byteptr_t
chunk_request(uint32_t size)
{
    size = CLAMP_CHUNK_SIZE(size);
    size = UINT32_PADDING(size, CHUNK_UNIT_ORDER);

    uint32_t index = (size >> CHUNK_UNIT_ORDER) - 1;

    uart_printf("[DEBUG] chunk_request - index: %d, size: %d\n", index, size);

    list_head_ptr_t head = CHUNK_TABLE(index);
    
    uart_printf("[DEBUG] chunk_request - head: 0x%x\n", head);
    
    list_head_ptr_t ptr = head->next;
    while (ptr != head && FRAME_IS_FULL(ptr)) { ptr = ptr->next; }

    uart_printf("[DEBUG] chunk_request - ptr: 0x%x\n", ptr);

    if (ptr == head) {
        byteptr_t frame = frame_alloc(1);
        INIT_LIST_HEAD((list_head_ptr_t) frame);
        init_frame(frame, (size >> CHUNK_UNIT_ORDER));
        list_add_tail((list_head_ptr_t) frame, head);

        uart_printf("[DEBUG] chunk_request - create frame: 0x%x, first: 0x%x\n", frame, head->next);

        ptr = (list_head_ptr_t) frame;
    }

    byteptr_t ret = frame_request_chunk((byteptr_t) ptr);
    return ret;
}


void
chunk_info(byteptr_t chunk)
{
    uart_printf("+- chunk addr: 0x%x, frame addr: 0x%x,", chunk, FRAME_ADDR(chunk));
    byteptr_t list = ADDR_TO_PTR(CHUNK_LIST(chunk));
    uint8_t size = list[CHUNK_SIZE_INDEX];
    uint8_t count = list[CHUNK_COUNT_INDEX];
    uart_printf(" chunk size: %d bytes, allocated count: %d, top: %d, free chunks: %d\n", size * 16, count, list[0], frame_count_free_chunks(chunk));
}

void
chunk_system_infro()
{
    uart_printf("==== CHUNK ALLOCATED INFO ====\n");
    for (uint32_t i = 0; i < CHUNK_MAX_COUNT; ++i) {
        list_head_ptr_t head = CHUNK_TABLE(i);
        if (!list_empty(head)) {
            uart_printf("chunk size: %d bytes:\n", (i+1) * 16);
            for (list_head_ptr_t ptr = head->next; ptr != head; ptr = ptr->next)
                chunk_info((byteptr_t) ptr);
        }
    }
}


uint32_t
chunk_max_size()
{
    return CHUNK_MAX_SIZE;
}


uint32_t
chunk_data_offset()
{
    return CHUNK_DATA_OFFSET;
}