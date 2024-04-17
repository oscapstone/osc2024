#include "multitask.h"

#include "interrupt.h"
#include "mem.h"
#include "uart1.h"
#include "utli.h"

static task_struct* current_thread = (task_struct*)0;
static task_struct* ready_que = (task_struct*)0;

static void kill_zombies() {}

static void idle_task(void* arg) {
  uart_puts("idle task start");
  while (1) {
    kill_zombies();
    schedule();
  }
  uart_puts("idle task end");
}

void init_sched_thread() {
  disable_interrupt();

  enable_interrupt();
}

task_struct* thread_create(start_routine_t start_routine, void* arg) {
  disable_interrupt();

  task_struct* new_thread = malloc(sizeof(task_status_t));
  if (!new_thread) {
    uart_puts("new thread allocation fail");
    return (task_struct*)0;
  }

  enable_interrupt();

  return new_thread;
}
