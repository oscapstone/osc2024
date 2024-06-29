#include "timer.h"

#include "mem.h"
#include "string.h"
#include "uart.h"

static timer_entry *head = 0;

void enable_timer_interrupt() {
  asm volatile(
      // Enable Counter-timer interrupt
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

void disable_timer_interrupt() {
  // Mask timer interrupt
  asm volatile(
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
    head->func(head->arg);   // Execute the callback function
    kfree((void *)head, 0);  // Free the memory allocated for the timer node
    head = head->next;       // Remove the head node after func finished
  }

  enable_timer_interrupt();
}

uint64_t timer_get_uptime() {
  uint64_t cntpct_el0 = 0, cntfrq_el0 = 0;
  asm volatile(
      "mrs %0, cntpct_el0;"
      "mrs %1, cntfrq_el0;"
      : "=r"(cntpct_el0), "=r"(cntfrq_el0));
  return cntpct_el0 / cntfrq_el0;
}

void timer_add(void (*callback)(void *), void *arg, int duration) {
  // Insert the new timer node into the linked list (sorted by time)

  timer_entry *timer = (timer_entry *)kmalloc(sizeof(timer_entry), 0);
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

static void show_timer_message(void *arg) {
  uart_putc(NEWLINE);
  uart_puts("[INFO] Timer message: ");
  uart_puts((char *)arg);
  uart_putc(NEWLINE);
  kfree(arg, 0);  // Free the memory allocated for the argument
}

void set_timer(const char *message, int duration) {
  timer_add((void (*)(void *))show_timer_message, (void *)message, duration);
}
