#include "include/allocator.h"
#include "include/buddy_system.h"
#include "include/dlist.h"
#include "include/heap.h"
#include "include/types.h"
#include "include/uart.h"

extern double_linked_node_t pools[];
extern const unsigned int SMALL_SIZES[];
extern frame_array_node_t frame_array[];

void memory_pool_init() {
  for (uint32_t i = 0; i < SMALL_SIZES_COUNT; i++) {
    double_linked_init(&pools[i]);
  }
}

int memory_pool_find_pool_index(uint32_t size) {
  for (uint32_t i = 0; i < SMALL_SIZES_COUNT; i++) {
    if (SMALL_SIZES[i] >= size) {
      return i;
    }
  }
  return -1;
}

void *memory_pool_allocator(uint32_t size) {
  int pool_index = memory_pool_find_pool_index(size);
  if (pool_index == -1) {
    uart_sendline("[Small Allocator] Error: Requested size %u "
                  "exceeds available pool sizes.\n",
                  size);
    return NULL;
  }

  double_linked_node_t *head_i = &pools[pool_index];
  double_linked_node_t *cur;
  frame_array_node_t *entry = NULL;

  double_linked_for_each(cur, head_i) {
    frame_array_node_t *cur_entry = (frame_array_node_t *)cur;
    if (cur_entry->slot_count > 0) {
      entry = cur_entry;
      uart_sendline("[Small Allocator] Using existing pool for allocation with "
                    "%u free slots.\n",
                    cur_entry->slot_count);
      break;
    }
  }

  uint32_t slots_size = SMALL_SIZES[pool_index];
  uint32_t slots = PAGE_SIZE / slots_size;
  if (!entry) {
    uint64_t new_page_addr = buddy_system_allocator(PAGE_SIZE);
    if (!new_page_addr) {
      uart_sendline("[Small Allocator] Error: Unable to allocate "
                    "page from buddy system.\n");
      return NULL;
    }
    entry = &frame_array[new_page_addr / PAGE_SIZE];
    double_linked_add_after(&entry->node, head_i);
    simple_memset(entry->slot_bitmap, 0xFF, (slots + 7) / 8);
    entry->slot_size = slots_size;
    entry->slot_count = slots;
    uart_sendline("[Small Allocator] New memory pool initialized and added to "
                  "pool list with %u slots.\n",
                  slots);
  }

  for (uint32_t i = 0; i < slots; i++) {
    uint32_t byte_index = i / 8;
    uint32_t bit_index = i % 8;
    if (entry->slot_bitmap[byte_index] & (1 << bit_index)) {
      entry->slot_bitmap[byte_index] &= ~(1 << bit_index);
      entry->slot_count--;
      void *allocated_address =
          (void *)(((uint64_t)entry->index * PAGE_SIZE) + (i * slots_size));
      uart_sendline("[Small Allocator] Allocated slot %u in frame %u, %u slots "
                    "remaining. Address: 0x%p\n",
                    i, entry->index, entry->slot_count, allocated_address);
      return allocated_address;
    }
  }
  return NULL;
}

void memory_pool_free(void *address) {
  if (!address) {
    uart_sendline("[Small Allocator] Error: Attempt to free a NULL pointer.\n");
    return;
  }

  uint64_t addr = (uint64_t)address;
  uint32_t page_index = addr / PAGE_SIZE;
  uint32_t offset_within_page = addr % PAGE_SIZE;

  frame_array_node_t *frame = &frame_array[page_index];
  if (!frame->slot_size) {
    uart_sendline("[Small Allocator] Error: Attempt to free an address not "
                  "managed by the allocator: 0x%p.\n",
                  address);
    return;
  }

  uint32_t slot_index = offset_within_page / frame->slot_size;
  uint32_t byte_index = slot_index / 8;
  uint32_t bit_index = slot_index % 8;
  if (frame->slot_bitmap[byte_index] & (1 << bit_index)) {
    uart_sendline("[Small Allocator] Error: Attempt to double free a slot for "
                  "address: 0x%p.\n",
                  address);
    return;
  }

  frame->slot_bitmap[byte_index] |= (1 << bit_index);
  frame->slot_count++;

  uart_sendline("[Small Allocator] Freed slot %u in frame %u, %u slots now "
                "free. Address: 0x%p\n",
                slot_index, page_index, frame->slot_count, address);

  if (frame->slot_count == (PAGE_SIZE / frame->slot_size)) {
    double_linked_remove(&frame->node);
    simple_memset(frame->slot_bitmap, 0, (frame->slot_count + 7) / 8);
    frame->slot_size = 0;
    frame->slot_count = 0;
    uart_sendline("[Small Allocator] All slots in frame %u freed, returning "
                  "the page to the buddy system.\n",
                  page_index);
    buddy_system_free(page_index * PAGE_SIZE);
  }
}