#include "scheduler.h"

#include "mem.h"
#include "uart.h"
#include "utils.h"

static int IDLE_PROCESS_PID;
static int thread_count = 0;
thread_struct *run_circle;

void sched_init() {
  uart_log(INFO, "Initializing Scheduler...\n");
  IDLE_PROCESS_PID = kcreate_thread(idle)->pid;
  asm volatile(
      "msr tpidr_el1, %0" ::"r"(run_circle)  // Software Thread ID Register
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
  thread->pid = thread_count++;
  thread->state = RUNNING;
  thread->stack = kmalloc(STACK_SIZE, 0);
  thread->user_stack = 0;  // alloc when needed (el0)
  memset(thread->sig_handlers, 0, sizeof(thread->sig_handlers));
  thread->sig_reg = 0;
  thread->sig_busy = 0;
  thread->context.lr = (unsigned long)func;
  thread->context.sp = (unsigned long)thread->stack + STACK_SIZE;
  thread->context.fp = (unsigned long)thread->stack + STACK_SIZE;
  add_thread(&run_circle, thread);
  return thread;
}

static void remove_thread(thread_struct **queue, thread_struct *thread) {
  if (*queue == thread) *queue = (thread->next == thread) ? 0 : thread->next;
  thread->next->prev = thread->prev;
  thread->prev->next = thread->next;
}

void list_tcircle() {
  thread_struct *thread = run_circle;
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
  } while (thread != run_circle);
}

void schedule() { switch_to(get_current(), get_current()->next); }

void kill_zombies() {
  thread_struct *next, *thread = run_circle;
  do {
    next = thread->next;
    if (thread->state == DEAD) {
      remove_thread(&run_circle, thread);
      kfree(thread->stack, 0);
      if (thread->user_stack) kfree(thread->user_stack, 0);
      kfree(thread, 0);
    }
    thread = next;
  } while (thread != run_circle);
}

void idle() {
  while (1) {
    kill_zombies();
    schedule();
  }
}

thread_struct *get_thread_by_pid(int pid) {
  thread_struct *thread = run_circle;
  do {
    if (thread->pid == pid) return thread;
    thread = thread->next;
  } while (thread != run_circle);
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
    thread_struct *thread = run_circle;
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
    } while (thread != run_circle);
    uart_log(WARN, "Nothing to kill.\n");
  }
  schedule();
}
