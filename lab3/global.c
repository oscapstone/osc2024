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

// dtb.c
char *dtb_ptr = NULL;

// heap.c
char *heap_ptr = NULL;

// uart.c
circular_buffer_t tx_buffer = {.head = 0, .tail = 0},
                  rx_buffer = {.head = 0, .tail = 0};

// timer.c
double_linked_node_t *timer_list_head = NULL;

// exception.c
irq_task_min_heap_t *irq_task_heap = NULL;
int current_irq_task_priority = 999;
