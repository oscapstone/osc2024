#include "timer.h"

#include "mem.h"
#include "str.h"
#include "uart.h"

static timerEntry *head = 0;

void init_timer() {
  asm volatile(
      "mov x0, 1\n"
      "msr cntp_ctl_el0, x0\n");

  uart_log(INFO, "enable_timer_interrupt()\n");
  enable_timer_interrupt();

  asm volatile(
      "mrs x0, cntfrq_el0\n"
      "msr cntp_tval_el0, x0\n");

  // required by lab5
  uint64_t tmp;
  asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
  tmp |= 1;
  asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));

  uart_log(INFO, "Timer IRQ frequency factor = ");
  uart_dec(TIMER_SPEED);
  uart_putc(NEWLINE);
}

void enable_timer_interrupt() {
  // Unmask the timer interrupt
  asm volatile(
      "mov x0, 2\n"
      "ldr x1, =%0\n"
      "str w0, [x1]\n"
      :
      : "i"(CORE0_TIMER_IRQCNTL)
      : "x0", "x1");
}

void disable_timer_interrupt() {
  // Mask timer interrupt
  asm volatile(
      "mov x0, 0\n"
      "ldr x1, =%0\n"
      "str w0, [x1]\n"
      :
      : "i"(CORE0_TIMER_IRQCNTL)
      : "x0", "x1");
}

void timer_irq_handler() {
  // Set up the next timer interrupt
  asm volatile(
      "mrs x0, cntfrq_el0\n"
      "lsr x0, x0, %0\n"
      "msr cntp_tval_el0, x0\n"
      :
      : "i"(TIMER_SPEED)  // frequency >> 5, aka. 1/32
      : "x0");

  // Check the timer queue
  while (head != 0 && timer_get_uptime() >= head->time) {
    head->func(head->arg);  // Execute the callback function
    kfree(head, SILENT);    // Free the memory allocated for the timer node
    head = head->next;      // Remove the head node after func finished
  }

  enable_timer_interrupt();
}

uint64_t timer_get_uptime() {
  uint64_t cntpct_el0 = 0, cntfrq_el0 = 0;
  asm volatile(
      "mrs %0, cntpct_el0\n"
      "mrs %1, cntfrq_el0\n"
      : "=r"(cntpct_el0), "=r"(cntfrq_el0));
  return cntpct_el0 / cntfrq_el0;
}

void timer_add(void (*callback)(void *), void *arg, int duration) {
  // Insert a new timer into the linked list (sorted by time)
  timerEntry *timer = kmalloc(sizeof(timerEntry), SILENT);
  timer->func = callback;
  timer->arg = arg;
  timer->time = timer_get_uptime() + duration;
  timer->next = 0;

  if (head == 0 || timer->time < head->time) {
    // Insert as the head of the list
    timer->next = head;
    head = timer;
    return;
  }

  timerEntry *current = head;
  while (current->next != 0 && current->next->time <= timer->time)
    current = current->next;
  timer->next = current->next;
  current->next = timer;
}

static void show_timer_message(void *arg) {
  uart_putc(NEWLINE);
  uart_log(INFO, "Timer message: ");
  uart_puts((char *)arg);
  uart_putc(NEWLINE);
  kfree(arg, SILENT);  // Free the memory allocated for arg
}

void set_timer(const char *msg, int duration) {
  timer_add((void (*)(void *))show_timer_message, (void *)msg, duration);
}

void set_timeup(int *timeup) { *timeup = 1; }
