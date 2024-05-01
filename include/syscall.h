#ifndef __SYSCALL_H__
#define __SYSCALL_H__


#include "stddef.h"

#define SYS_GET_TASKID 0
#define SYS_UART_READ 1
#define SYS_UART_WRITE 2
#define SYS_EXEC 3
#define SYS_FORK 4
#define SYS_EXIT 5

#define SYSCALL_NUM 6
#define SYSCALL_SUCCESS 0
#define SYSCALL_ERROR -1


/** 
 * Because include syscall.h will copy all the content in header file,
 * considering the portability, we should split the content in syscall.h.
 * Content above is the content for systemcall.S and content below is for syscall.c.
 * */
#ifndef __ASSEMBLER__ // Content below is not included in assembly file. (__ASSEMBLY__ is defined in assembly file)


#include "stdlib.h"
#include "stdint.h"


struct trapframe {
    uint64_t x[31];
    uint64_t sp; // sp_el0, user space stack pointer
    uint64_t pc; // elr_el1
    uint64_t pstate; // spsr_el1
};

/* for systemcall.S, user space function */
/* system call: get current task id. */
extern int get_taskid();
/* system call: uart_read(buf, size). Use uart_getc() to fill the buf for given size. */
extern size_t uart_read(char *buf, size_t size);
/* system call: uart_write(buf, size). Output the contents of the buf for given size. */
extern size_t uart_write(const char *buf, size_t size);
/* system call: exec(func). For user space to execute the function. */
extern void exec(void (*func)());
/* system call: For user space to fork current task. */
extern int fork();
/* system call: exit(status). */
extern void exit(int status);

/* for syscall.c, kernel space handler function */
int sys_get_taskid(struct trapframe *trapframe);
int sys_uart_read(struct trapframe *trapframe);
int sys_uart_write(struct trapframe *trapframe);
int sys_exec(struct trapframe *trapframe);
int sys_fork(struct trapframe *trapframe);
int sys_exit(struct trapframe *trapframe);

typedef int (*syscall_t)(struct trapframe *);
extern syscall_t sys_call_table[SYSCALL_NUM];

#endif // __ASSEMBLER__
#endif // __SYSCALL_H__