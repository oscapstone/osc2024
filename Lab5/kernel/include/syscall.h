#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYS_GETPID      0
#define SYS_UARTREAD    1
#define SYS_UARTWRITE   2
#define SYS_EXEC        3
#define SYS_FORK        4
#define SYS_EXIT        5
#define SYS_MBOXCALL    6
#define SYS_KILL        7

#include "sched.h"
#include "uart.h"
#include "cpio.h"
#include "thread.h"
#include "util.h"

typedef struct trap_frame trap_frame_t;

int sys_getpid();
uint32_t sys_uart_read(char buf[], uint32_t size);
uint32_t sys_uart_write(const char buf[], uint32_t size);
int sys_exec(const char* name, char *const argv[]);
int sys_fork(trap_frame_t* tpf);
void sys_exit();
int sys_mbox_call(unsigned char ch, unsigned int* mbox);
void sys_kill(int pid);

void fork_test();

#endif