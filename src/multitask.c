#include "multitask.h"

#include "cpio_.h"
#include "interrupt.h"
#include "mem.h"
#include "shell.h"
#include "string.h"
#include "uart1.h"
#include "utli.h"

task_struct* current_thread;
task_struct* threads;
static ts_deque ready_que;

void ready_que_push_back(task_struct* thread) {
  OS_enter_critical();
  task_struct* t = ready_que.tail->prev;
  t->next = thread;
  ready_que.tail->prev = thread;
  thread->prev = t;
  thread->next = ready_que.tail;
  OS_exit_critical();
}

static inline task_struct* ready_que_pop_front() {
  OS_enter_critical();
  task_struct* ret = ready_que.head->next;
  ready_que.head->next = ret->next;
  ret->next->prev = ready_que.head;
  ret->prev = ret->next = (task_struct*)0;
  OS_exit_critical();
  return ret;
}

static void reclaim() {
  OS_enter_critical();
  for (int i = 0; i < PROC_NUM; i++) {
    if (threads[i].status == THREAD_DEAD) {
      uart_send_string("\r\nidle task: reclaimed thread pid: ");
      uart_int(threads[i].pid);
      uart_send_string("\r\n");
      free(threads[i].usr_stk);
      free(threads[i].ker_stk);
      free(threads[i].data);
      threads[i].status = THREAD_FREE;
    }
  }
  OS_exit_critical();
}

static void idle_task() {
  while (1) {
    reclaim();
    // uint64_t ttmp;
    // asm volatile("mrs %0, daif" : "=r"(ttmp));
    // uart_send_string("DAIF_: ");
    // uart_hex_64(ttmp);
    // uart_send_string("\r\n");
    // wait_usec(100000);
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
  }
  task_exit();
}

void schedule() {
  if (current_thread->status == THREAD_READY) {
    ready_que_push_back(current_thread);
  }
  task_struct* to_switch = ready_que_pop_front();
#ifdef DEBUG
  uart_send_string("current_thread->pid: ");
  uart_int(current_thread->pid);
  uart_send_string("\r\n");
  uart_send_string("to_switch->pid: ");
  uart_int(to_switch->pid);
  uart_send_string("\r\n\n");
#endif

  disable_interrupt();
  task_struct* tmp = current_thread;
  current_thread = to_switch;

  // uint64_t ttmp;
  // asm volatile("mrs %0, daif" : "=r"(ttmp));
  // uart_send_string("DAIF: ");
  // uart_hex_64(ttmp);
  // uart_send_string("\r\n");

  switch_to(&tmp->cpu_context, &to_switch->cpu_context);
}

static void init_thread_pool() {
  threads = (task_struct*)malloc(sizeof(task_struct) * PROC_NUM);
  for (int i = 0; i < PROC_NUM; i++) {
    threads[i].status = THREAD_FREE;
    threads[i].pid = i + 1;
    threads[i].prev = threads[i].next = (task_struct*)0;
    threads[i].usr_stk = threads[i].ker_stk = (void*)0;
    threads[i].data = (void*)0;
  }
};

static void init_ready_queue() {
  task_struct* tmp_ts = (task_struct*)malloc(sizeof(task_struct));
  tmp_ts->prev = tmp_ts->next = tmp_ts;
  ready_que.head = ready_que.tail = tmp_ts;
}

void init_sched_thread() {
  init_thread_pool();
  init_ready_queue();
  task_struct* start_thread = &threads[0];
  start_thread->status = THREAD_READY;
  start_thread->ker_stk = malloc(KER_STK_SZ);

  void* cur_sp = get_cur_sp();
  uint64_t cur_stk_sz = 0x40000 - (uint64_t)cur_sp;
  void* dst_sp = start_thread->ker_stk + KER_STK_SZ - (cur_stk_sz);
  memcpy(dst_sp, cur_sp, cur_stk_sz);
  asm volatile("mov sp, %0" : "=r"(dst_sp));

  set_current_thread(&start_thread->cpu_context);
  current_thread = start_thread;
  thread_create(&idle_task);
}

task_struct* get_free_thread() {
  task_struct* ret = (task_struct*)0;
  OS_enter_critical();
  for (int i = 0; i < PROC_NUM; i++) {
    if (threads[i].status == THREAD_FREE) {
      threads[i].status = THREAD_READY;
      ret = (threads + i);
      break;
    }
  }
  OS_exit_critical();
  return ret;
}

task_struct* thread_create(start_routine_t start_routine) {
  task_struct* new_thread = get_free_thread();
  if (!new_thread) {
    uart_puts("new thread allocation fail");
    return (task_struct*)0;
  }

  new_thread->ker_stk = malloc(KER_STK_SZ);
  new_thread->cpu_context.fp = new_thread->cpu_context.sp =
      (uint64_t)(new_thread->ker_stk + KER_STK_SZ);
  new_thread->cpu_context.lr = (uint64_t)start_routine;
  ready_que_push_back(new_thread);

  return new_thread;
}

void startup_thread_exec(char* file) {
  uint32_t file_sz;
  char* usr_prog = cpio_load(file, &file_sz);
  if (!usr_prog) {
    return;
  }

  current_thread->data = malloc(file_sz);
  current_thread->data_size = file_sz;
  memcpy(current_thread->data, usr_prog, file_sz);
  current_thread->usr_stk = malloc(USR_STK_SZ);
  current_thread->cpu_context.lr = (uint64_t)current_thread->data;
  current_thread->cpu_context.fp = current_thread->cpu_context.sp =
      (uint64_t)current_thread->ker_stk + KER_STK_SZ;

#ifdef DEBUG
  uart_send_string("thread_exec usr sp: ");
  uart_hex_64((uint64_t)current_thread->usr_stk + USR_STK_SZ);
  uart_send_string("\r\n");
  uart_send_string("thread_exec ker sp: ");
  uart_hex_64(current_thread->cpu_context.sp);
  uart_send_string("\r\n");
#endif

  asm volatile(
      "msr spsr_el1, xzr\n\t"
      "msr elr_el1, %0\n\t"
      "msr sp_el0, %1\n\t"
      "mov sp, %2\n\t"
      "eret\n\t" ::"r"(current_thread->cpu_context.lr),
      "r"(current_thread->usr_stk + USR_STK_SZ),
      "r"(current_thread->cpu_context.sp));
}

void thread_exec_func(start_routine_t start_routine) {
  current_thread->ker_stk = malloc(KER_STK_SZ);
  current_thread->usr_stk = malloc(USR_STK_SZ);
  current_thread->cpu_context.lr = (uint64_t)start_routine;
  current_thread->cpu_context.fp = current_thread->cpu_context.sp =
      (uint64_t)current_thread->ker_stk + KER_STK_SZ;

  uart_send_string("thread_exec usr sp: ");
  uart_hex_64((uint64_t)current_thread->usr_stk + USR_STK_SZ);
  uart_send_string("\r\n");
  uart_send_string("thread_exec ker sp: ");
  uart_hex_64(current_thread->cpu_context.sp);
  uart_send_string("\r\n");

  asm volatile(
      "msr spsr_el1, xzr\n\t"
      "msr elr_el1, %0\n\t"
      "msr sp_el0, %1\n\t"
      "mov sp, %2\n\t"
      "eret\n\t" ::"r"(current_thread->cpu_context.lr),
      "r"(current_thread->usr_stk + USR_STK_SZ),
      "r"(current_thread->cpu_context.sp));
}

void task_exit() {
  OS_enter_critical();
  current_thread->status = THREAD_DEAD;
  OS_exit_critical();
  schedule();
}