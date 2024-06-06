#include "mem.h"

#include "alloc.h"
#include "interrupt.h"
#include "math_.h"
#include "uart1.h"
#include "utli.h"

extern char __kernel_start;
extern char __kernel_end;
extern char __pt_start;
extern char __pt_end;
extern void* cpio_start_addr;
extern void* cpio_end_addr;
extern void* _dtb_ptr_start;
extern void* _dtb_ptr_end;
extern void* startup_alloc_start;
extern void* startup_alloc_end;

static uint32_t chunk_sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
frame_entry* frame_entry_arr;
static frame_node* free_frame_lists[MAX_ORDER + 1];
static chunk_entry* chunk_entry_arr;
static chunk_node* free_chunk_lists[CHUNK_SIZE_TYPES];

static void* idx2address(uint32_t idx) {
  return (void*)(MEM_START + (uint64_t)idx * FRAME_SIZE);
}

uint32_t address2idx(void* address) {
  return ((uint64_t)address - MEM_START) / FRAME_SIZE;
}

static void add_free_frame(uint32_t frame_idx, uint32_t order) {
  frame_node* n;
  if (!frame_entry_arr[frame_idx].node) {
    n = (frame_node*)simple_malloc(sizeof(frame_node));
  } else {
    n = frame_entry_arr[frame_idx].node;
  }

  n->addr = idx2address(frame_idx);
  free_frame_lists[order]->prev = n;
  n->next = free_frame_lists[order];
  n->prev = (frame_node*)0;
  free_frame_lists[order] = n;
  frame_entry_arr[frame_idx].order = order;
  frame_entry_arr[frame_idx].node = n;
  frame_entry_arr[frame_idx].status = FREE;
}

static void del_free_frame(uint32_t frame_idx) {
  frame_node* n = frame_entry_arr[frame_idx].node;
  uint32_t order = frame_entry_arr[frame_idx].order;
  frame_entry_arr[frame_idx].order = BUDDY;

  if (n == free_frame_lists[order]) {
    free_frame_lists[order] = free_frame_lists[order]->next;
    free_frame_lists[order]->prev = (frame_node*)0;
  } else {
    n->prev->next = n->next;
    n->next->prev = n->prev;
  }
}

static void add_free_chunk(void* addr, uint32_t alloc_idx) {
  chunk_node* n = (chunk_node*)simple_malloc(sizeof(chunk_node));
  n->addr = addr;
  n->next = free_chunk_lists[alloc_idx];
  free_chunk_lists[alloc_idx] = n;
}

static uint32_t get_least_alloc_order(uint32_t frame_num) {
  uint32_t val = 1;
  while (val < frame_num) {
    val *= 2;
  }
  return log2(val);
}

static int32_t get_alloc_chunk_size(uint32_t n) {
  int32_t i = 0;
  for (i = 0; i < CHUNK_SIZE_TYPES; i++) {
    if (chunk_sizes[i] >= n) {
      return i;
    }
  }
  return -1;
}

static void init_frames() {
  frame_entry_arr =
      (frame_entry*)simple_malloc(sizeof(frame_entry) * FRAME_CNT);

  for (int i = 0; i < FRAME_CNT; i++) {
    frame_entry_arr[i].order = 0;
    frame_entry_arr[i].status = FREE;
    frame_entry_arr[i].node = (frame_node*)0;
    frame_entry_arr[i].ref_cnt = 0;
  }

  for (int i = 0; i <= MAX_ORDER; i++) {
    free_frame_lists[i] = (frame_node*)simple_malloc(sizeof(frame_node));
    free_frame_lists[i]->prev = free_frame_lists[i]->next = (frame_node*)0;
    free_frame_lists[i]->addr = (void*)0;
  }
}

static void init_chunks() {
  chunk_entry_arr =
      (chunk_entry*)simple_malloc(sizeof(chunk_entry) * FRAME_CNT);
  for (int i = 0; i < FRAME_CNT; i++) {
    chunk_entry_arr[i].is_mem_pool = 0;
  }
  for (int i = 0; i < CHUNK_SIZE_TYPES; i++) {
    free_chunk_lists[i] = (chunk_node*)0;
  }
}

static void mem_reserve(void* start, void* end) {
  uint32_t st_frame_idx = address2idx(start);
  uint32_t end_frame_idx = address2idx((void*)((uint64_t)end - 1));
  for (int i = st_frame_idx; i <= end_frame_idx; i++) {
    frame_entry_arr[i].status = ALLOCATED;
  }
}

static void merge_initial_frames() {
  for (int i = 0; i <= MAX_ORDER; i++) {
    uint32_t frame_idx = 0;
    for (;;) {
      uint32_t buddy_idx = frame_idx ^ (1 << i);

      if (buddy_idx >= FRAME_CNT) {
        break;
      }

      if (frame_entry_arr[frame_idx].status == FREE &&
          frame_entry_arr[buddy_idx].status == FREE &&
          frame_entry_arr[frame_idx].order == i &&
          frame_entry_arr[buddy_idx].order == i) {
        frame_entry_arr[frame_idx].order++;
        frame_entry_arr[buddy_idx].order = BUDDY;
      }

      frame_idx += (1 << (i + 1));
      if (frame_idx >= FRAME_CNT) {
        break;
      }
    }
  }

  for (int i = 0; i < FRAME_CNT; i++) {
    if (frame_entry_arr[i].status == FREE && frame_entry_arr[i].order >= 0) {
      add_free_frame(i, frame_entry_arr[i].order);
    }
  }
}

void init_mem() {
  uart_send_string("SPIN_TABLE_START: ");
  uart_hex_64((uint64_t)SPIN_TABLE_START);
  uart_send_string("\r\n");
  uart_send_string("SPIN_TABLE_end: ");
  uart_hex_64((uint64_t)SPIN_TABLE_END);
  uart_send_string("\r\n");
  uart_send_string("kernel start address: ");
  uart_hex_64((uint64_t)&__kernel_start);
  uart_send_string("\r\n");
  uart_send_string("kernel end address: ");
  uart_hex_64((uint64_t)&__kernel_end);
  uart_send_string("\r\n");
  uart_send_string("page table start address: ");
  uart_hex_64((uint64_t)&__pt_start);
  uart_send_string("\r\n");
  uart_send_string("page table end address: ");
  uart_hex_64((uint64_t)&__pt_end);
  uart_send_string("\r\n");
  uart_send_string("dtb start address: ");
  uart_hex_64((uint64_t)_dtb_ptr_start);
  uart_send_string("\r\n");
  uart_send_string("dtb end address: ");
  uart_hex_64((uint64_t)_dtb_ptr_end);
  uart_send_string("\r\n");
  uart_send_string("cpio_start_addr: ");
  uart_hex_64((uint64_t)cpio_start_addr);
  uart_send_string("\r\n");
  uart_send_string("cpio_end_addr: ");
  uart_hex_64((uint64_t)cpio_end_addr);
  uart_send_string("\r\n");
  uart_send_string("startup_alloc_start: ");
  uart_hex_64((uint64_t)startup_alloc_start);
  uart_send_string("\r\n");
  uart_send_string("startup_alloc_end: ");
  uart_hex_64((uint64_t)startup_alloc_end);
  uart_send_string("\r\n");

  init_frames();
  init_chunks();

  mem_reserve(SPIN_TABLE_START, SPIN_TABLE_END);
  mem_reserve((void*)&__kernel_start, (void*)&__kernel_end);
  mem_reserve((void*)&__pt_start, (void*)&__pt_end);
  mem_reserve(_dtb_ptr_start, _dtb_ptr_end);
  mem_reserve(cpio_start_addr, cpio_end_addr);
  mem_reserve(startup_alloc_start, startup_alloc_end);

  merge_initial_frames();
}

static void* alloc_frame(uint32_t frame_num) {
  uint32_t least_alloc_order = get_least_alloc_order(frame_num);
  uint32_t real_alloc_order = least_alloc_order;
  while (real_alloc_order <= MAX_ORDER &&
         !(free_frame_lists[real_alloc_order]->addr)) {
    real_alloc_order++;
  }
  if (real_alloc_order > MAX_ORDER) {
    uart_puts("No available frame!");
    return (void*)0;
  }

  frame_node* n = free_frame_lists[real_alloc_order];
  free_frame_lists[real_alloc_order] = n->next;
  uint32_t frame_idx = address2idx(n->addr);

  // release redundant memory block
  while (real_alloc_order > least_alloc_order) {
    real_alloc_order--;
    uint32_t buddy_idx = frame_idx ^ (1 << real_alloc_order);
    add_free_frame(buddy_idx, real_alloc_order);
  }

  frame_entry_arr[frame_idx].status = ALLOCATED;
  frame_entry_arr[frame_idx].order = real_alloc_order;
  return idx2address(frame_idx);
}

static void* alloc_chunk(uint32_t size) {
  int32_t alloc_idx = get_alloc_chunk_size(size);
  uint32_t alloc_size = chunk_sizes[alloc_idx];

  if (alloc_idx < 0) {
    uart_puts("alloc_chunk error: invaild size");
    return (void*)0;
  }

  if (!free_chunk_lists[alloc_idx]) {
    void* frame_addr = alloc_frame(1);
    uint32_t frame_idx = address2idx(frame_addr);
    chunk_entry_arr[frame_idx].is_mem_pool = 1;
    chunk_entry_arr[frame_idx].size = alloc_size;
    chunk_entry_arr[frame_idx].free_chunk_cnt = FRAME_SIZE / alloc_size;
    for (int i = 0; i < FRAME_SIZE; i += alloc_size) {
      add_free_chunk(frame_addr + i, alloc_idx);
    }
  }

  void* alloc_addr = free_chunk_lists[alloc_idx]->addr;
  uint32_t frame_idx = address2idx(alloc_addr);
  chunk_entry_arr[frame_idx].free_chunk_cnt--;
  free_chunk_lists[alloc_idx] = free_chunk_lists[alloc_idx]->next;

  return alloc_addr;
}

void* malloc(uint32_t size) {
  void* addr = (void*)0;
  OS_enter_critical();
  if (size != 0) {
    if (size <= MAX_CHUNK_SIZE) {
      addr = alloc_chunk(size);
    } else {
      uint32_t frame_num = (size + FRAME_SIZE - 1) / FRAME_SIZE;
      addr = alloc_frame(frame_num);
    }
  }
  OS_exit_critical();
  return addr;
}

static void free_chunk(void* addr) {
  // objects from the same page frame have a common prefix address.
  uint32_t frame_idx = address2idx(addr);
  if (!chunk_entry_arr[frame_idx].is_mem_pool) {
    return;
  }

  int32_t alloc_idx = get_alloc_chunk_size(chunk_entry_arr[frame_idx].size);
  if (alloc_idx < 0) {
    uart_puts("free_chunk error: invaild size");
    return;
  }

  chunk_entry_arr[frame_idx].free_chunk_cnt++;

  if (chunk_entry_arr[frame_idx].free_chunk_cnt ==
      FRAME_SIZE / chunk_entry_arr[frame_idx].size) {
    chunk_entry_arr[frame_idx].is_mem_pool = 0;
    free_chunk_lists[alloc_idx] = (chunk_node*)0;
  } else {
    add_free_chunk(addr, alloc_idx);
  }
}

static void free_frame(void* addr) {
  uint32_t frame_idx = address2idx(addr);
  if (chunk_entry_arr[frame_idx].is_mem_pool) {
    return;
  }
  uint32_t buddy_idx = frame_idx ^ (1 << frame_entry_arr[frame_idx].order);
  // coalesce blocks
  while (
      frame_entry_arr[frame_idx].order < MAX_ORDER &&
      (frame_entry_arr[frame_idx].order == frame_entry_arr[buddy_idx].order) &&
      frame_entry_arr[buddy_idx].status == FREE) {
    del_free_frame(buddy_idx);
    frame_entry_arr[frame_idx].order++;
    if (buddy_idx < frame_idx) {
      frame_entry_arr[buddy_idx].order = frame_entry_arr[frame_idx].order;
      frame_entry_arr[frame_idx].order = BUDDY;
      frame_idx = buddy_idx;
    }
    buddy_idx = frame_idx ^ (1 << frame_entry_arr[frame_idx].order);
  }
  add_free_frame(frame_idx, frame_entry_arr[frame_idx].order);
}

void free(void* addr) {
  if (!addr) {
    return;
  }
  OS_enter_critical();
  free_chunk(addr);
  free_frame(addr);
  OS_exit_critical();
}