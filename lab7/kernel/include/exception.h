#ifndef __EXCEPTION_H
#define __EXCEPTION_H
#include "syscall.h"

extern void return_from_fork();

void enable_interrupt();
void disable_interrupt();
void exception_entry();
void el1_sync_handler_entry();
void el0_svc_handler_entry(struct ucontext* trapframe);
void el0_da_handler_entry();
void el0_ia_handler_entry();
void irq_handler_entry();

void rx_task();
void tx_task();
void timer_task();

void sys_getpid(struct ucontext* trapframe);
void sys_uartread(struct ucontext* trapframe);
void sys_uartwrite(struct ucontext* trapframe);
void sys_exec(struct ucontext* trapframe);
void sys_fork(struct ucontext* trapframe);
void sys_exit(struct ucontext* trapframe);
void sys_mbox_call(struct ucontext *trapframe);
void sys_kill(struct ucontext *trapframe);
void sys_signal(struct ucontext *trapframe);
void sys_signal_kill(struct ucontext *trapframe);
void sys_mmap(struct ucontext *trapframe);

void sys_open(struct ucontext *trapframe);
void sys_close(struct ucontext *trapframe);
void sys_write(struct ucontext *trapframe);
void sys_read(struct ucontext *trapframe);
void sys_mkdir(struct ucontext *trapframe);
void sys_mount(struct ucontext *trapframe);
void sys_chdir(struct ucontext *trapframe);
void sys_lseek64(struct ucontext *trapframe);

void sys_sigreturn(struct ucontext *trapframe);


#endif