#include "include/allocator.h"
#include "include/buddy_system.h"
#include "include/cpio.h"
#include "include/dlist.h"
#include "include/exception.h"
#include "include/shell.h"
#include "include/types.h"
#include "include/uart.h"

// shell.c
char cmd[MAX_CMD_LEN];
int preempt = 0;

// cpio.c
cpio_newc_header_t *cpio_header = NULL;
char *cpio_end = NULL;

// dtb.c
char *dtb_ptr = NULL;

// heap.c
char *heap_ptr = NULL;
startup_memory_block_t *startup_memory_block_table_start = NULL;
startup_memory_block_t *startup_memory_block_table_end = NULL;

// uart.c
circular_buffer_t tx_buffer = {.head = 0, .tail = 0},
                  rx_buffer = {.head = 0, .tail = 0};

// timer.c
double_linked_node_t *timer_list_head = NULL;

// exception.c
irq_task_min_heap_t *irq_task_heap = NULL;
int current_irq_task_priority = 999;

// buddy_system.c
buddy_system_node_t buddy_system[MAX_LEVEL + 1];
frame_array_node_t frame_array[TOTAL_MEMORY / PAGE_SIZE];

// allocator.c
double_linked_node_t pools[SMALL_SIZES_COUNT];
const unsigned int SMALL_SIZES[SMALL_SIZES_COUNT] = {32,  64,  128,
                                                     256, 512, 1024};