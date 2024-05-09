#include "include/timer.h"
#include "include/dlist.h"
#include "include/exception.h"
#include "include/heap.h"
#include "include/types.h"
#include "include/uart.h"
#include "include/utils.h"

extern double_linked_node_t *timer_list_head;
extern irq_task_min_heap_t *irq_task_heap;

void timer_list_init() {
  timer_list_head = simple_malloc(sizeof(double_linked_node_t), 0);
  if (timer_list_head == NULL) {
    uart_sendline("Error: Failed to allocate memory for timer list head.\n");
    return;
  }
  double_linked_init(timer_list_head);
}

void core_timer_enable() {
  asm volatile("msr cntp_ctl_el0, %0" : : "r"(1));
  *CORE0_TIMER_IRQCNTL = 0x2;
};

void core_timer_disable() { *CORE0_TIMER_IRQCNTL = 0x0; };

timer_task_t *create_timer_task(int secs, void *callback, const char *arg,
                                int priority) {
  timer_task_t *task = simple_malloc(sizeof(timer_task_t), 0);
  if (task == NULL) {
    uart_sendline("Error: Failed to allocate memory for timer task.\n");
    return NULL;
  }
  unsigned long cntpct_el0 = 0;
  unsigned long cntfrq_el0 = 0;
  __asm__ __volatile__("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
  __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));
  task->trigger_time = cntpct_el0 + cntfrq_el0 * secs;
  task->callback = callback;
  const char *prefix = "\n[TIMER IRQ] ";
  int total_length = strlen(prefix) + strlen(arg) + 1;
  char *buf = simple_malloc(total_length, 0);
  if (buf == NULL) {
    uart_sendline("Error: Failed to allocate memory for timer task arg.\n");
    return NULL;
  }
  strcpy(buf, prefix);
  strcat(buf, arg);
  task->callback_arg = buf;
  task->priority = priority;
  return task;
}

void add_timer_task(timer_task_t *new_task) {
  if (new_task == NULL) {
    uart_sendline("Error: Cannot add a NULL timer task.\n");
    return;
  }
  double_linked_node_t *current;
  timer_task_t *entry;
  double_linked_for_each(current, timer_list_head) {
    entry = (timer_task_t *)current;
    if (new_task->trigger_time < entry->trigger_time) {
      break;
    }
  }
  double_linked_add_before(&new_task->node, current);
  core_timer_update();
}

void core_timer_handler() {
  if (double_linked_is_empty(timer_list_head)) {
    __asm__ __volatile__(
        "mrs x1, cntpct_el0\n" // Read current counter value into x1
        "mrs x2, cntfrq_el0\n" // Read the frequency of the counter into x2
        "mov x3, #10000\n"
        "mul x2, x2, x3\n"          // x2 = cntfrq_el0 * 10000
        "add x1, x1, x2\n"          // x1 = cntpct_el0 + cntfrq_el0 * 10000
        "msr cntp_cval_el0, x1\n"); // Set the compare value register to x1
    return;
  }
  add_timer_task_to_irq_heap();
}

void add_timer_task_to_irq_heap() {
  timer_task_t *task = (timer_task_t *)timer_list_head->next;
  double_linked_remove(timer_list_head->next);
  core_timer_update();
  core_timer_enable();
  irq_task_min_heap_push(
      irq_task_heap,
      create_irq_task(task->callback, task->callback_arg, task->priority));
}

void core_timer_update() {
  unsigned long current_time, freq, cval;
  asm volatile("mrs %0, cntpct_el0" : "=r"(current_time));
  asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
  if (double_linked_is_empty(timer_list_head)) {
    cval = current_time + freq * 10000;
  } else {
    timer_task_t *next_task = (timer_task_t *)(timer_list_head->next);
    if (next_task->trigger_time > current_time) {
      cval = next_task->trigger_time;
    } else {
      cval = current_time;
    }
  }
  asm volatile("msr cntp_cval_el0, %0" : : "r"(cval));
}