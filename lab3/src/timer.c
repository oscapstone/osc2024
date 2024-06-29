#include "timer.h"

#include "malloc.h"
#include "string.h"
#include "uart.h"

static int timeup = 0;

typedef struct __timer_t {
  void (*func)(void *);
  void *arg;
  int time;
  struct __timer_t *next;
} timer_entry;

static timer_entry *head = 0;

void timer_enable_interrupt() {
  asm volatile(
      // Enable Counter-timer  interrupt
      "mov x0, 1;"
      "msr cntp_ctl_el0, x0;"  // Physical Timer Control register

      // Set the Counter-timer frequency to 1 second
      "mrs x0, cntfrq_el0;"     // Frequency register
      "msr cntp_tval_el0, x0;"  // Physical Timer TimerValue register

      // Unmask Counter-timer interrupt
      "mov x0, 2;"
      "ldr x1, =%0;"
      "str w0, [x1];"  // IRQ Enable Register
      :
      : "i"(CORE0_TIMER_IRQCNTL)
      : "x0", "x1"  // registers modified
  );
}

void timer_disable_interrupt() {
  asm volatile(
      // Mask timer interrupt
      "mov x0, 0;"
      "ldr x1, =%0;"
      "str w0, [x1];"
      :
      : "i"(CORE0_TIMER_IRQCNTL)
      : "x0", "x1");
}

void timer_irq_handler() {
  // Set up 1 second core timer interrupt
  asm volatile(
      "mrs x0, cntfrq_el0;"     // Frequency register
      "msr cntp_tval_el0, x0;"  // Physical Timer TimerValue register
  );

  // Check the timer queue
  while (head != 0 && timer_get_uptime() >= head->time) {
    head->func(head->arg);  // Execute the callback function
    head = head->next;      // Remove the head node after func finished
  }

  timer_enable_interrupt();
}

uint64_t timer_get_uptime() {
  uint64_t cntpct_el0 = 0;
  uint64_t cntfrq_el0 = 0;
  asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
  asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));
  return cntpct_el0 / cntfrq_el0;
}

void timer_add(void (*callback)(void *), void *arg, int duration) {
  // Insert the new timer node into the linked list (sorted by time)

  timer_entry *timer = (timer_entry *)simple_malloc(sizeof(timer_entry));
  timer->func = callback;
  timer->arg = arg;
  timer->time = timer_get_uptime() + duration;
  timer->next = 0;

  if (head == 0 || timer->time < head->time) {
    // Insert at the beginning of the list
    timer->next = head;
    head = timer;
    return;
  }

  timer_entry *current = head;
  while (current->next != 0 && current->next->time <= timer->time)
    current = current->next;
  timer->next = current->next;
  current->next = timer;
}

void set_timer(const char *message, int duration) {
  timer_add((void (*)(void *))uart_puts, (void *)message, duration);
}

void reset_timeup() { timeup = 0; }

int get_timeup() { return timeup; }

void set_timeup() { timeup = 1; }