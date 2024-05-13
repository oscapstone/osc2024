#ifndef __EXCEPTION_H
#define __EXCEPTION_H
#include "syscall.h"

extern void return_from_fork();

void enable_interrupt();
void disable_interrupt();
void exception_entry();
void sync_handler_entry();
void el0_svc_handler_entry(struct trapframe* trapframe);
void irq_handler_entry();

void rx_task();
void tx_task();
void timer_task();

void sys_getpid(struct trapframe* trapframe);
void sys_uartread(struct trapframe* trapframe);
void sys_uartwrite(struct trapframe* trapframe);
void sys_exec(struct trapframe* trapframe);
void sys_fork(struct trapframe* trapframe);
void sys_exit(struct trapframe* trapframe);
void sys_mbox_call(struct trapframe *trapframe);
void sys_kill(struct trapframe *trapframe);

#endif