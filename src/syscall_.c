#include "syscall_.h"

#include "interrupt.h"
#include "multitask.h"
#include "uart1.h"
#include "utli.h"

extern task_struct* current_thread;
extern task_struct* threads;

uint64_t sys_getpid(trapframe_t* tpf) {
  tpf->x[0] = current_thread->pid;
  return tpf->x[0];
}

uint32_t sys_uartread(trapframe_t* tpf, char buf[], uint32_t size) {
  uint32_t i;
  for (i = 0; i < size; i++) {
    buf[i] = uart_read_async();
  }
  tpf->x[0] = i;
  return i;
}

uint32_t sys_uartwrite(trapframe_t* tpf, const char buf[], uint32_t size) {
  uint32_t i;
  for (i = 0; i < size; i++) {
    uart_write_async(buf[i]);
  }
  tpf->x[0] = i;
  return i;
}

uint32_t exec(trapframe_t* tpf, const char* name, char* const argv[]) {}

uint32_t fork(trapframe_t* tpf) { task_struct* new_thread = get_free_thread(); }

void exit(trapframe_t* tpf) {
  UNUSED(tpf);
  task_exit();
}

void kill(uint32_t pid) {
  if (pid < 0 || pid >= PROC_NUM || threads[pid].status == THREAD_FREE) {
    return;
  }

  OS_enter_critical();
  threads[pid].status = THREAD_DEAD;
  OS_exit_critical();
}