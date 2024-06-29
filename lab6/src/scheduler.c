#include "scheduler.h"

#include "irq.h"
#include "mem.h"
#include "start.h"
#include "uart.h"
#include "utils.h"
#include "virtm.h"

static int IDLE_PROCESS_PID;
static int thread_count = 0;
thread_struct *running_tcircle;

void sched_init() {
  uart_log(INFO, "Initializing Scheduler...\n");
  IDLE_PROCESS_PID = kcreate_thread(idle)->pid;
  asm volatile(
      "msr tpidr_el1, %0" ::"r"(running_tcircle)  // Software Thread ID Register
  );
  uart_log(INFO, "Scheduler Initialized.\n");
}

static void add_thread(thread_struct **queue, thread_struct *thread) {
  if (*queue == 0) {
    *queue = thread;
    thread->next = thread;
    thread->prev = thread;
  } else {
    // prev <- thread -> *queue
    thread->next = *queue;
    thread->prev = (*queue)->prev;
    // prev -> thread <- *queue
    (*queue)->prev->next = thread;
    (*queue)->prev = thread;
  }
}

thread_struct *kcreate_thread(void (*func)()) {
  thread_struct *thread = kmalloc(sizeof(thread_struct), 0);
  uart_log(INFO, "Creating thread PID: ");
  uart_dec(thread->pid = thread_count++);
  uart_putc(NEWLINE);

  thread->start = func ? func : 0;
  thread->size = 0;
  thread->state = RUNNING;

  thread->stack = kmalloc(STACK_SIZE, 0);
  uart_log(INFO, "Acquired thread kernel stack at ");
  uart_hex((uintptr_t)thread->stack);
  uart_putc(NEWLINE);
  thread->user_stack = 0;  // alloc when needed (el0)

  memset(thread->sig_handlers, 0, sizeof(thread->sig_handlers));
  thread->sig_reg = 0;
  thread->sig_busy = 0;

  // Added for lab6
  memset(thread->pgd = kmalloc(PAGE_SIZE, 0), 0, PAGE_SIZE);
  uart_log(INFO, "Acquired thread pgd at ");
  uart_hex((uintptr_t)thread->pgd);
  uart_putc(NEWLINE);

  thread->context.lr = (unsigned long)func;
  thread->context.sp = (unsigned long)thread->stack + STACK_SIZE;
  thread->context.fp = (unsigned long)thread->stack + STACK_SIZE;
  add_thread(&running_tcircle, thread);
  return thread;
}

static void remove_thread(thread_struct **queue, thread_struct *thread) {
  if (*queue == thread) *queue = (thread->next == thread) ? 0 : thread->next;
  thread->next->prev = thread->prev;
  thread->prev->next = thread->next;
}

void list_tcircle() {
  thread_struct *thread = running_tcircle;
  uart_log(INFO, "All thread(s) in queue:\n");
  do {
    uart_log(INFO, "pid = ");
    uart_dec(thread->pid);
    if (thread->sig_handlers[9] > 0) uart_putc('*');
    uart_puts(", sp: ");
    uart_hex((uintptr_t)thread->context.sp);
    if (thread->pid == get_current()->pid) uart_puts(" <- current");
    uart_putc(NEWLINE);
    thread = thread->next;
  } while (thread != running_tcircle);
}

void schedule() {
  switch_mm((unsigned long)TO_PHYS(get_current()->next->pgd));
  switch_to(get_current(), get_current()->next);
}

void kill_zombies() {
  thread_struct *next, *thread = running_tcircle;
  do {
    next = thread->next;
    if (thread->state == DEAD) {
      remove_thread(&running_tcircle, thread);
      kfree(thread->stack, 0);
      if (thread->user_stack) kfree(thread->user_stack, 0);
      kfree(thread->pgd, 0);
      kfree(thread, 0);
    }
    thread = next;
  } while (thread != running_tcircle);
}

void idle() {
  while (1) {
    kill_zombies();
    schedule();
  }
}

thread_struct *get_thread_by_pid(int pid) {
  thread_struct *thread = running_tcircle;
  do {
    if (thread->pid == pid) return thread;
    thread = thread->next;
  } while (thread != running_tcircle);
  return 0;
}

void kill_current_thread() {
  get_current()->state = DEAD;
  schedule();
}

void kill_thread_by_pid(int pid) {
  if (pid == IDLE_PROCESS_PID) {
    uart_log(WARN, "Cannot kill idle process.\n");
  } else {
    thread_struct *thread = running_tcircle;
    do {
      if (thread->pid == pid) {
        uart_log(INFO, "Killing pid ");
        uart_dec(pid);
        uart_putc(NEWLINE);
        thread->state = DEAD;
        schedule();
        return;
      }
      thread = thread->next;
    } while (thread != running_tcircle);
    uart_log(WARN, "Nothing to kill.\n");
  }
  schedule();
}
