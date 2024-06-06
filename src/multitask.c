#include "multitask.h"

#include "alloc.h"
#include "cpio_.h"
#include "interrupt.h"
#include "mem.h"
#include "shell.h"
#include "string.h"
#include "uart1.h"
#include "utli.h"
#include "vm.h"

task_struct* threads;
static ts_deque ready_que;
extern int32_t lock_cnt;
extern char __kernel_start;

void ready_que_del_node(task_struct* thread) {
  task_struct_node* n = &thread->ts_node;
  if (n->prev == (task_struct_node*)0 && n->next == (task_struct_node*)0) {
    return;
  }
  OS_enter_critical();
  n->prev->next = n->next;
  n->next->prev = n->prev;
  n->prev = n->next = (task_struct_node*)0;
  OS_exit_critical();
}

void ready_que_push_back(task_struct* thread) {
  OS_enter_critical();
  task_struct_node* n = ready_que.tail->prev;
  n->next = &thread->ts_node;
  ready_que.tail->prev = &thread->ts_node;
  thread->ts_node.prev = n;
  thread->ts_node.next = ready_que.tail;
  OS_exit_critical();
}

task_struct* ready_que_pop_front() {
  OS_enter_critical();
  task_struct_node* ret = ready_que.head->next;
  ready_que.head->next = ret->next;
  ret->next->prev = ready_que.head;
  ret->prev = ret->next = (task_struct_node*)0;
  OS_exit_critical();
  return ret->ts;
}

static void init_thread_struct(task_struct* ts, uint32_t pid) {
  memset(ts, 0, sizeof(task_struct));
  ts->pid = pid;
  ts->status = THREAD_FREE;
  ts->ts_node.ts = ts;
  ts->sig_handlers[SYSCALL_SIGKILL].func = sig_kill_default_handler;
  ts->mm.pgd = KER_PGD_ADDR;
}

static void reclaim() {
  OS_enter_critical();
  for (int i = 0; i < PROC_NUM; i++) {
    if (threads[i].status == THREAD_DEAD) {
      if (threads[i].mm.pgd != KER_PGD_ADDR) {
        vm_free(&threads[i]);
      }

      free(threads[i].ker_stk);
      free(threads[i].sig_stk);

      init_thread_struct(&threads[i], i + 1);

      uart_send_string("\r\nidle task: reclaimed thread pid: ");
      uart_int(threads[i].pid);
      uart_send_string("\r\n");
    }
  }
  OS_exit_critical();
}

void idle_task() {
  while (1) {
    reclaim();
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
  if (lock_cnt != 0) {
    uart_puts("schedule warning: lock_cnt != 0");
    return;
  }

  if (current_thread->status == THREAD_RUNNING) {
    current_thread->status = THREAD_READY;
    ready_que_push_back(current_thread);
  }
  task_struct* to_switch;
  to_switch = ready_que_pop_front();
  to_switch->status = THREAD_RUNNING;
  if (current_thread->pid == to_switch->pid) {
    return;
  }
#ifdef DEBUG
  print_timestamp();
  uart_send_string("current_thread->pid: ");
  uart_int(current_thread->pid);
  uart_send_string(", to_switch->pid: ");
  uart_int(to_switch->pid);
  uart_send_string("\r\n");
  uart_send_string("current_thread->mm.pgd: ");
  uart_hex_64(current_thread->mm.pgd);
  uart_send_string(", to_switch->mm.pgd: ");
  uart_hex_64(to_switch->mm.pgd);
  uart_send_string("\r\n\n");
#endif
  // set_current_pgd(current_thread->mm.pgd);
  switch_to(current_thread, to_switch);
}

static void init_thread_pool() {
  threads = (task_struct*)malloc(sizeof(task_struct) * PROC_NUM);
  for (int i = 0; i < PROC_NUM; i++) {
    init_thread_struct(&threads[i], i + 1);
  }
};

static void init_ready_queue() {
  task_struct_node* n = (task_struct_node*)malloc(sizeof(task_struct_node));
  n->prev = n->next = n;
  ready_que.head = ready_que.tail = n;
}

void init_sched_thread() {
  lock_cnt++;

  init_thread_pool();
  init_ready_queue();
  task_struct* start_thread = &threads[0];
  start_thread->status = THREAD_RUNNING;
  start_thread->ker_stk = malloc(KER_STK_SZ);

  void* cur_sp = get_cur_sp();
  uint64_t cur_stk_sz = (uint64_t)(&__kernel_start) - (uint64_t)cur_sp;
  void* dst_sp = start_thread->ker_stk + KER_STK_SZ - (cur_stk_sz);
  memcpy(dst_sp, cur_sp, cur_stk_sz);
  asm volatile("mov sp, %0" : "=r"(dst_sp));

  set_current_thread(start_thread);

  // thread_create(&idle_task);

  lock_cnt--;
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

void user_thread_exec() {
  uint32_t file_sz;
  char* usr_prog = cpio_load("vm.img", &file_sz);
  if (!usr_prog) {
    return;
  }
  current_thread->data_size = file_sz;
  current_thread->data = malloc(file_sz);
  map_pages(current_thread, CODE, (uint64_t)current_thread->data, USR_CODE_ADDR,
            file_sz, VM_PROT_READ | VM_PROT_EXEC, MAP_ANONYMOUS);
  memcpy(current_thread->data, usr_prog, file_sz);
  current_thread->usr_stk = malloc(USR_STK_SZ);

  map_pages(current_thread, STACK, (uint64_t)current_thread->usr_stk,
            USR_STK_ADDR, USR_STK_SZ, VM_PROT_READ | VM_PROT_WRITE,
            MAP_ANONYMOUS);
  map_pages(current_thread, IO, phy2vir(IO_PM_START_ADDR), IO_PM_START_ADDR,
            IO_PM_END_ADDR - IO_PM_START_ADDR, VM_PROT_READ | VM_PROT_WRITE,
            MAP_ANONYMOUS);

  uart_send_string("current_thread->mm.pgd: ");
  uart_hex_64(current_thread->mm.pgd);
  uart_send_string("\r\n");
  uart_send_string("current_thread->data: ");
  uart_hex_64((uint64_t)current_thread->data);
  uart_send_string("\r\n");
  uart_send_string("current_thread->usr_stk: ");
  uart_hex_64((uint64_t)current_thread->usr_stk + USR_STK_SZ);
  uart_send_string("\r\n");
  uart_send_string("current_thread->ker_stk: ");
  uart_hex_64((uint64_t)current_thread->ker_stk + KER_STK_SZ);
  uart_send_string("\r\n");

  set_current_pgd(current_thread->mm.pgd);

  asm volatile(
      "msr spsr_el1, xzr\n\t"
      "msr elr_el1, %0\n\t"
      "msr sp_el0, %1\n\t"
      "mov sp, %2\n\t"
      "eret\n\t" ::"r"(USR_CODE_ADDR),
      "r"(USR_STK_ADDR + USR_STK_SZ),
      "r"(current_thread->ker_stk + KER_STK_SZ));
}

task_struct* thread_create(start_routine_t start_routine) {
  task_struct* new_thread = get_free_thread();
  new_thread->ker_stk = malloc(KER_STK_SZ);
  new_thread->cpu_context.fp = new_thread->cpu_context.sp =
      (uint64_t)(new_thread->ker_stk + KER_STK_SZ);
  new_thread->cpu_context.lr = (uint64_t)start_routine;

  ready_que_push_back(new_thread);

  return new_thread;
}

void task_exit() {
  OS_enter_critical();
  current_thread->status = THREAD_DEAD;
  OS_exit_critical();
  schedule();
}