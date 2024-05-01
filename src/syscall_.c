#include "syscall_.h"

#include "interrupt.h"
#include "mem.h"
#include "multitask.h"
#include "peripherals/mbox.h"
#include "string.h"
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

uint32_t exec(trapframe_t* tpf, const char* name, char* const argv[]) {
  uint32_t file_sz;
  char* prog = cpio_load(name, &file_sz);
  if (!prog) {
    tpf->x[0] = -1;
    return -1;
  }

  memcpy(current_thread->data, prog, file_sz);
  current_thread->data_size = file_sz;
  tpf->elr_el1 = (uint64_t)current_thread->data;
  tpf->sp_el0 = (uint64_t)current_thread->usr_stk + USR_STK_SZ;

  tpf->x[0] = 0;
  return 0;
}

uint32_t fork(trapframe_t* tpf) {
  task_struct* child = get_free_thread();
  if (!child) {
    uart_puts("new process allocation fail");
    tpf->x[0] = -1;
    return -1;
  }

  OS_enter_critical();
  task_struct* parent = current_thread;
  OS_exit_critical();

  child->ker_stk = malloc(KER_STK_SZ);
  memcpy(child->ker_stk, parent->ker_stk, KER_STK_SZ);
  child->usr_stk = malloc(USR_STK_SZ);
  memcpy(child->usr_stk, parent->usr_stk, USR_STK_SZ);
  child->data = malloc(parent->data_size);
  child->data_size = parent->data_size;
  memcpy(child->data, parent->data, parent->data_size);

#ifdef DEBUG
  uart_send_string("parent->ker_stk: ");
  uart_hex_64(parent->ker_stk);
  uart_send_string(", child->ker_stk: ");
  uart_hex_64(child->ker_stk);
  uart_send_string("\r\n");
  uart_send_string("parent->usr_stk: ");
  uart_hex_64(parent->usr_stk);
  uart_send_string(", child->usr_stk: ");
  uart_hex_64(child->usr_stk);
  uart_send_string("\r\n");
  uart_send_string("parent->data: ");
  uart_hex_64(parent->data);
  uart_send_string(", child->data: ");
  uart_hex_64(child->data);
  uart_send_string("\r\n");
#endif

  store_cpu_context(&parent->cpu_context);
  if (current_thread->pid == parent->pid) {
    memcpy(&child->cpu_context, &parent->cpu_context, sizeof(cpu_context_t));
    child->cpu_context.sp =
        (uint64_t)child->ker_stk +
        (parent->cpu_context.sp - (uint64_t)parent->ker_stk);
    child->cpu_context.fp =
        (uint64_t)child->ker_stk +
        (parent->cpu_context.fp - (uint64_t)parent->ker_stk);
#ifdef DEBUG
    uart_send_string("parent->cpu_context.sp: ");
    uart_hex_64(parent->cpu_context.sp);
    uart_send_string(", child->cpu_context.sp: ");
    uart_hex_64(child->cpu_context.sp);
    uart_send_string("\r\n");
    uart_send_string("parent->cpu_context.fp: ");
    uart_hex_64(parent->cpu_context.fp);
    uart_send_string(", child->cpu_context.fp: ");
    uart_hex_64(child->cpu_context.fp);
    uart_send_string("\r\n");
    uart_send_string("parent->cpu_context.lr: ");
    uart_hex_64(parent->cpu_context.lr);
    uart_send_string(", child->cpu_context.lr: ");
    uart_hex_64(child->cpu_context.lr);
    uart_send_string("\r\n");
#endif

    tpf->x[0] = child->pid;
    ready_que_push_back(child);
    return child->pid;
  }
  trapframe_t* child_tpf =
      (trapframe_t*)(child->ker_stk +
                     ((uint64_t)tpf - (uint64_t)parent->ker_stk));
  child_tpf->sp_el0 =
      (uint64_t)child->usr_stk + (tpf->sp_el0 - (uint64_t)parent->usr_stk);
  child_tpf->elr_el1 = tpf->elr_el1;
#ifdef DEBUG
  uart_send_string("parent_tpf: ");
  uart_hex_64((uint64_t)tpf);
  uart_send_string(", child_tpf: ");
  uart_hex_64((uint64_t)child_tpf);
  uart_send_string("\r\n");
  uart_send_string("parent_tpf->sp_el0: ");
  uart_hex_64(tpf->sp_el0);
  uart_send_string(", child_tpf->sp_el0 : ");
  uart_hex_64(child_tpf->sp_el0);
  uart_send_string("\r\n");
  uart_send_string("parent_tpf->elr_el1: ");
  uart_hex_64(tpf->elr_el1);
  uart_send_string(", child_tpf->elr_el1: ");
  uart_hex_64(child_tpf->elr_el1);
  uart_send_string("\r\n");
#endif

  child_tpf->x[0] = 0;
  return 0;
}

void exit(trapframe_t* tpf) {
  UNUSED(tpf);
  task_exit();
}

uint32_t sys_mbox_call(trapframe_t* tpf, uint8_t ch, uint32_t* mbox) {
  uint64_t r = (((uint64_t)((uint64_t)mbox) & ~0xF) | (ch & 0xF));

  do {
    asm volatile("nop");
  } while (*MBOX_STATUS & MBOX_FULL);

  *MBOX_WRITE = r;

  while (1) {
    do {
      asm volatile("nop");
    } while (*MBOX_STATUS & MBOX_EMPTY);

    if (r == *MBOX_READ) {
      OS_exit_critical();
      tpf->x[0] = (mbox[1] == MBOX_CODE_BUF_RES_SUCC);
      return (mbox[1] == MBOX_CODE_BUF_RES_SUCC);
    }
  }
  OS_exit_critical();
  tpf->x[0] = 0;
  return 0;
}

void kill(uint32_t pid) {
  if (pid <= 0 || pid > PROC_NUM || threads[pid - 1].status == THREAD_FREE) {
    return;
  }

  OS_enter_critical();
  threads[pid - 1].status = THREAD_DEAD;
  OS_exit_critical();
}