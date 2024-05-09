#ifndef BUDDY_SYSTEM_H
#define BUDDY_SYSTEM_H

#include "dlist.h"
#include "types.h"

#define PAGE_SIZE 0x1000 // 4KB
#define MAX_LEVEL 14
#define TOTAL_MEMORY 0x3C000000

// for demo
// #define MAX_LEVEL 3
// #define TOTAL_MEMORY 3 * 8 * 0x1000

typedef struct buddy_system_node {
  uint32_t count;
  char *bitmap;
  double_linked_node_t head;
} buddy_system_node_t;

typedef struct frame_array_node {
  double_linked_node_t node;
  uint32_t size;
  uint32_t index;
  char *slot_bitmap;
  uint32_t slot_size;
  uint32_t slot_count;
} frame_array_node_t;

void buddy_system_init();
uint32_t buddy_system_find_level(uint32_t size);
uint32_t size_to_power_of_two(uint32_t size);
uint64_t buddy_system_allocator(uint32_t size);
void buddy_system_free(uint64_t address);
void buddy_system_print_bitmap();
void buddy_system_print_freelists(int show_bitmap);
void buddy_system_reserve_memory(uint64_t start, uint64_t end);
void buddy_system_reserve_memory_init();
void buddy_system_merge_bottom_up();
void buddy_system_freelists_init();

#endif /* BUDDY_SYSTEM_H */