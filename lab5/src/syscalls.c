#include "syscalls.h"

#include "initramfs.h"
#include "irq.h"
#include "mbox.h"
#include "mem.h"
#include "scheduler.h"
#include "str.h"
#include "traps.h"
#include "uart.h"
#include "utils.h"

int sys_getpid() { return get_current()->pid; }

size_t sys_uart_read(char *buf, size_t size) {
  int i = 0;
  while (i < size) buf[i++] = uart_getc();
  return i;
}

size_t sys_uart_write(const char *buf, size_t size) {
  int i = 0;
  while (i < size) uart_putc(buf[i++]);
  return i;
  return size;
}

int sys_exec(const char *name, trap_frame *tf) {
  initramfs_sys_exec(name, tf);
  return 0;
}

int sys_fork(trap_frame *tf) {
  disable_interrupt();  // prevent schedule premature fork

  thread_struct *parent = get_current();
  thread_struct *child = kcreate_thread(0);

  // Handling kernel stack (incl. tf from parent)
  memcpy(child->stack, parent->stack, STACK_SIZE);
  // get child's trap frame from kstack via parent's offset
  size_t ksp_offset = (uintptr_t)tf - (uintptr_t)parent->stack;
  trap_frame *child_tf = (trap_frame *)(child->stack + ksp_offset);
  child->context.sp = (uintptr_t)child_tf;  // set child's ksp

  // Handling user stack
  if (parent->user_stack) {
    child->user_stack = kmalloc(STACK_SIZE, 0);
    memcpy(child->user_stack, parent->user_stack, STACK_SIZE);
    size_t usp_offset = tf->sp_el0 - (uintptr_t)parent->user_stack;
    child_tf->sp_el0 = (uintptr_t)child->user_stack + usp_offset;
  }

  // Copy signal handlers
  memcpy(child->sig_handlers, parent->sig_handlers,
         sizeof(parent->sig_handlers));

  child->context.lr = (uintptr_t)child_ret_from_fork;  // traps.S
  child_tf->x0 = 0;  // ret value of fork() for child

  enable_interrupt();
  return child->pid;
}

void sys_exit(int status) {
  uart_log(status ? WARN : INFO, "Exiting process ...\n");
  kill_current_thread();
}

void sys_sigreturn(trap_frame *tf) {
  // Restore the sigframe
  memcpy(tf, get_current()->sig_tf, sizeof(trap_frame));
  kfree(get_current()->sig_tf, 0);
  kfree(get_current()->sig_stack, 0);
  get_current()->sig_busy = 0;
  return;  // Jump to the previous context (user program) after eret
}
