#include "include/buddy_system.h"
#include "include/allocator.h"
#include "include/cpio.h"
#include "include/dtb.h"
#include "include/heap.h"
#include "include/types.h"
#include "include/uart.h"
#include "include/utils.h"

extern buddy_system_node_t buddy_system[];
extern frame_array_node_t frame_array[];
extern startup_memory_block_t *startup_memory_block_table_start;
extern startup_memory_block_t *startup_memory_block_table_end;

void buddy_system_init() {
  uart_sendline("Buddy system initialization...\n");
  uart_sendline("============================\n");
  uart_sendline("Total memory: 0x%p bytes.\n", TOTAL_MEMORY);
  for (uint32_t i = 0; i <= MAX_LEVEL; i++) {
    uint32_t blocks = TOTAL_MEMORY / (PAGE_SIZE << i);
    uart_sendline("Level %u:\n", i);
    uart_sendline("  Total blocks: %u\n", blocks);
    buddy_system[i].bitmap = simple_malloc((blocks + 7) / 8, 0);
    double_linked_init(&buddy_system[i].head);
    if (i == 0) {
      buddy_system[i].count = blocks;
      simple_memset(buddy_system[i].bitmap, 0xFF, (blocks + 7) / 8);
    }
  }
  for (uint32_t i = 0; i < TOTAL_MEMORY / PAGE_SIZE; i++) {
    double_linked_init(&frame_array[i].node);
    frame_array[i].size = 0;
    frame_array[i].index = i;
    frame_array[i].slot_bitmap =
        simple_malloc((PAGE_SIZE / SMALL_SIZE + 7) / 8, 0);
    frame_array[i].slot_count = 0;
    frame_array[i].slot_size = 0;
  }
  uart_sendline("============================\n");
  buddy_system_reserve_memory_init();
  buddy_system_merge_bottom_up();
  buddy_system_freelists_init();
}

uint32_t buddy_system_find_level(uint32_t size) {
  uint32_t level = 0;
  uint32_t block_size = PAGE_SIZE;
  while (block_size < size && level < MAX_LEVEL) {
    block_size <<= 1;
    level++;
  }
  return level;
}

uint32_t size_to_power_of_two(uint32_t size) {
  uint32_t power = 0;
  while ((1 << power) < size) {
    power++;
  }
  return 1 << power;
}

uint64_t buddy_system_allocator(uint32_t size) {
  uint32_t level = buddy_system_find_level(size);
  uart_sendline("[Allocator] Requesting size %u bytes, mapping to level %u.\n",
                size, level);
  size = size_to_power_of_two(size);
  size = size < PAGE_SIZE ? PAGE_SIZE : size;
  for (uint32_t current_level = level; current_level <= MAX_LEVEL;
       current_level++) {
    uart_sendline("[Allocator] Checking level %u, available blocks %u.\n",
                  current_level, buddy_system[current_level].count);

    if (buddy_system[current_level].count > 0) {
      frame_array_node_t *frame_node =
          (frame_array_node_t *)buddy_system[current_level].head.next;
      uint32_t block_index = frame_node->index / (1 << current_level);

      uart_sendline("[Allocator] Found free block at level %u, frame %u.\n",
                    current_level, block_index * (1 << current_level));
      buddy_system[current_level].bitmap[block_index / 8] &=
          ~(1 << (block_index % 8));
      buddy_system[current_level].count--;
      double_linked_remove((double_linked_node_t *)frame_node);

      uint32_t split_index = block_index;
      while (current_level > level) {
        uint32_t next_level = current_level - 1;
        uint32_t buddy_index_1 = split_index * 2;
        uint32_t buddy_index_2 = buddy_index_1 + 1;

        uart_sendline("[Allocator] Splitting from level %u to %u, frame %u "
                      "used, frame %u free.\n",
                      current_level, next_level,
                      buddy_index_1 * (1 << next_level),
                      buddy_index_2 * (1 << next_level));
        buddy_system[next_level].bitmap[buddy_index_2 / 8] |=
            (1 << (buddy_index_2 % 8));
        buddy_system[next_level].count++;
        double_linked_add_before(
            (double_linked_node_t
                 *)&frame_array[buddy_index_2 * (1 << next_level)],
            &buddy_system[next_level].head);

        current_level = next_level;
        split_index = buddy_index_1;
      }

      uint64_t address = split_index * (PAGE_SIZE << level);
      uart_sendline("[Allocator] Allocated block at address 0x%p, level %u.\n",
                    address, level);
      frame_array[split_index * (1 << level)].size = size;
      return address;
    }
  }
  uart_sendline(
      "[Allocator] Error: No suitable block found for size %u bytes.\n", size);
  return 0;
}

void buddy_system_free(uint64_t address) {
  frame_array_node_t *frame_node = &frame_array[address / PAGE_SIZE];
  uint32_t size = frame_node->size;
  if (size == 0) {
    uart_sendline("[Free] Error: Attempt to free unallocated or "
                  "uninitialized block at address 0x%p.\n",
                  address);
    return;
  }
  frame_array[address / PAGE_SIZE].size = 0;

  uint32_t level = buddy_system_find_level(size);
  uint32_t block_index = address / (PAGE_SIZE << level);
  uart_sendline("[Free] Freeing block at level %u, address 0x%p with "
                "size %u bytes, frame %u.\n",
                level, address, size, block_index * (1 << level));
  buddy_system[level].bitmap[block_index / 8] |= (1 << (block_index % 8));
  double_linked_add_before((double_linked_node_t *)frame_node,
                           &buddy_system[level].head);
  buddy_system[level].count++;

  while (level < MAX_LEVEL) {
    uint32_t block_size = PAGE_SIZE << level;
    uint32_t parent_index = block_index / 2;
    uint32_t buddy_index = block_index ^ 1;

    if (buddy_index < (TOTAL_MEMORY / block_size) &&
        (buddy_system[level].bitmap[buddy_index / 8] &
         (1 << (buddy_index % 8))) != 0) {
      uart_sendline("[Free] Found free buddy at level %u, merging frame %u and "
                    "frame %u.\n",
                    level, block_index * (1 << level),
                    buddy_index * (1 << level));

      buddy_system[level].bitmap[block_index / 8] &= ~(1 << (block_index % 8));
      double_linked_remove(
          (double_linked_node_t *)&frame_array[block_index * (1 << level)]);
      buddy_system[level].bitmap[buddy_index / 8] &= ~(1 << (buddy_index % 8));
      double_linked_remove(
          (double_linked_node_t *)&frame_array[buddy_index * (1 << level)]);
      buddy_system[level].count -= 2;

      block_index = parent_index;
      level++;
      buddy_system[level].bitmap[block_index / 8] |= (1 << (block_index % 8));
      double_linked_add_before(
          (double_linked_node_t *)&frame_array[block_index * (1 << level)],
          &buddy_system[level].head);
      buddy_system[level].count++;
    } else {
      uart_sendline("[Free] No free buddy found at level %u, stopping "
                    "merge process.\n",
                    level);
      break;
    }
  }
}

void buddy_system_print_bitmap() {
  for (uint32_t level = 0; level <= MAX_LEVEL; level++) {
    uint32_t blocks = TOTAL_MEMORY / (PAGE_SIZE << level);
    uart_sendline("[Bitmap] Level %u (%u blocks, %u bytes each): ", level,
                  blocks, PAGE_SIZE << level);
    for (uint32_t i = 0; i < (blocks + 7) / 8; i++) {
      for (uint32_t bit = 0; bit < 8; bit++) {
        if (8 * i + bit < blocks) {
          uart_sendline("%u",
                        buddy_system[level].bitmap[i] & (1 << bit) ? 1 : 0);
        }
      }
    }
    uart_sendline("\n");
  }
}

void buddy_system_print_freelists(int show_bitmap) {
  if (show_bitmap) {
    buddy_system_print_bitmap();
  }
  for (uint32_t level = 0; level <= MAX_LEVEL; level++) {
    uart_sendline("[Freelists] Level %u: ", level);
    uart_sendline("[");
    int first = 1;
    double_linked_node_t *cur;
    double_linked_for_each(cur, &buddy_system[level].head) {
      if (!first) {
        uart_sendline(", ");
      }
      first = 0;
      frame_array_node_t *frame_node = (frame_array_node_t *)cur;
      // uart_sendline("0x%p", (uint64_t)(frame_node->index * PAGE_SIZE));
      uart_sendline("%u", frame_node->index);
    }
    uart_sendline("]\n");
  }
}

void buddy_system_reserve_memory(uint64_t start, uint64_t end) {
  uint32_t start_index = start / PAGE_SIZE;
  uint32_t end_index = end / PAGE_SIZE;
  for (uint32_t idx = start_index; idx <= end_index; idx++) {
    buddy_system[0].bitmap[idx / 8] &= ~(1 << idx % 8);
    buddy_system[0].count--;
    frame_array[idx].size = PAGE_SIZE;
  }
}

void buddy_system_reserve_memory_init() {
  startup_memory_block_t *current = startup_memory_block_table_start;
  while (current != NULL) {
    uart_sendline(
        "[Reserve] Reserving memory from 0x%p to 0x%p, size: %u bytes.\n",
        current->address, current->address + current->size, current->size);
    buddy_system_reserve_memory(current->address,
                                current->address + current->size);
    current = current->next;
  }
}

void buddy_system_merge_bottom_up() {
  for (uint32_t level = 0; level < MAX_LEVEL; level++) {
    uint32_t blocks = TOTAL_MEMORY / (PAGE_SIZE << level);
    for (uint32_t block_index = 0; block_index < blocks; block_index += 2) {
      if (block_index + 1 >= blocks)
        continue;
      uint32_t byte_index1 = block_index / 8;
      uint32_t bit_index1 = block_index % 8;
      uint32_t byte_index2 = (block_index + 1) / 8;
      uint32_t bit_index2 = (block_index + 1) % 8;
      if ((buddy_system[level].bitmap[byte_index1] & (1 << bit_index1)) &&
          (buddy_system[level].bitmap[byte_index2] & (1 << bit_index2))) {
        buddy_system[level].bitmap[byte_index1] &= ~(1 << bit_index1);
        buddy_system[level].bitmap[byte_index2] &= ~(1 << bit_index2);
        buddy_system[level].count -= 2;
        buddy_system[level + 1].bitmap[(block_index / 2) / 8] |=
            (1 << ((block_index / 2) % 8));
        buddy_system[level + 1].count++;
      }
    }
  }
}

void buddy_system_freelists_init() {
  for (uint32_t level = 0; level <= MAX_LEVEL; level++) {
    uint32_t blocks = TOTAL_MEMORY / (PAGE_SIZE << level);
    for (uint32_t block_index = 0; block_index < blocks; block_index++) {
      if (buddy_system[level].bitmap[block_index / 8] &
          (1 << (block_index % 8))) {
        double_linked_add_before(
            (double_linked_node_t *)&frame_array[block_index * (1 << level)],
            &buddy_system[level].head);
      }
    }
  }
}