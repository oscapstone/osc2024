#include "multitask.h"

#include "interrupt.h"
#include "mem.h"
#include "shell.h"
#include "uart1.h"
#include "utli.h"

static task_struct* current_thread;
static ts_deque ready_que;
static ts_que reclaim_que;

static inline void ready_que_push_back(task_struct* thread) {
  OS_enter_critical();
  if (!ready_que.head) {
    ready_que.head = ready_que.tail = thread;
  } else {
    ready_que.tail->next = thread;
    ready_que.tail = thread;
  }
  OS_exit_critical();
}

static inline task_struct* ready_que_pop_front() {
  OS_enter_critical();
  task_struct* ret = ready_que.head;
  ready_que.head = ret->next;
  ret->next = (task_struct*)0;
  OS_exit_critical();
  return ret;
}

static inline void reclaim_que_push_front(task_struct* thread) {
  OS_enter_critical();
  thread->next = reclaim_que.top;
  reclaim_que.top = thread;
  OS_exit_critical();
}

static void reclaim() {
  OS_enter_critical();
  task_struct* pts = reclaim_que.top;
  while (pts) {
    task_struct* next = pts->next;
#ifdef DEBUG
    uart_send_string("reclaimed thread pid: ");
    uart_int(pts->pid);
    uart_send_string("\r\n");
#endif
    free(pts->usr_stk);
    free(pts->ker_stk);
    free(pts->data);
    free(pts);
    pts = next;
  }
  reclaim_que.top = (task_struct*)0;
  OS_exit_critical();
}

static void idle_task() {
  while (1) {
#ifdef DEBUG
    uart_puts("idle task start");
#endif
    reclaim();
#ifdef DEBUG
    uart_puts("idle task end");
#endif
    schedule();
  }
}

void foo() {
  for (int i = 0; i < 10; ++i) {
    uart_send_string("Thread id: ");
    uart_int(current_thread->pid);
    uart_send_string(" ");
    uart_int(i);
    uart_send_string("\r\n");
    wait_usec(1000000);
    schedule();
  }
  current_thread->status = DEAD;
  schedule();
}

void schedule() {
  OS_enter_critical();

  if (current_thread->status == READY) {
    ready_que_push_back(current_thread);
  } else {
    reclaim_que_push_front(current_thread);
  }

#ifdef DEBUG
  uart_send_string("ready_que.head pid: ");
  uart_int(ready_que.head->pid);
  uart_send_string("\r\n");
  uart_send_string("ready_que.tail pid: ");
  uart_int(ready_que.tail->pid);
  uart_send_string("\r\n");
#endif

  task_struct* to_switch = ready_que_pop_front();
#ifdef DEBUG
  uart_send_string("current_thread->pid: ");
  uart_int(current_thread->pid);
  uart_send_string("\r\n");
  uart_send_string("to_switch->pid: ");
  uart_int(to_switch->pid);
  uart_send_string("\r\n");
#endif

  task_struct* tmp = current_thread;
  current_thread = to_switch;
  switch_to(&tmp->cpu_context, &to_switch->cpu_context);
  OS_exit_critical();
}

void init_sched_thread() {
  OS_enter_critical();

  task_struct* start_thread = (task_struct*)malloc(sizeof(task_struct));
  start_thread->status = READY;
  start_thread->usr_stk = start_thread->ker_stk = (void*)0;
  start_thread->next = (task_struct*)0;
  start_thread->data = (void*)0;
  start_thread->data_size = 0;
  start_thread->pid = (uint64_t)&start_thread->cpu_context;

  reclaim_que.top = (task_struct*)0;
  ready_que.head = ready_que.tail = (task_struct*)0;
  set_current_thread(&start_thread->cpu_context);
  current_thread = start_thread;

  thread_create(&idle_task);

  OS_exit_critical();
}

task_struct* thread_create(start_routine_t start_routine) {
  OS_enter_critical();

  task_struct* new_thread = malloc(sizeof(task_struct));
  if (!new_thread) {
    uart_puts("new thread allocation fail");
    OS_exit_critical();
    return (task_struct*)0;
  }

  new_thread->status = READY;
  new_thread->usr_stk = (void*)0;
  new_thread->ker_stk = malloc(KER_STK_SZ);
  new_thread->data = (void*)0;
  new_thread->data_size = 0;
  new_thread->cpu_context.fp = new_thread->cpu_context.sp =
      (uint64_t)(new_thread->ker_stk + KER_STK_SZ);
  new_thread->cpu_context.lr = (uint64_t)start_routine;
  new_thread->next = (task_struct*)0;
  new_thread->pid = (uint64_t)(&new_thread->cpu_context);
  ready_que_push_back(new_thread);

  OS_exit_critical();
  return new_thread;
}
