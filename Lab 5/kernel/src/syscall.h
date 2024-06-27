#ifndef	__SYSCALL_H__
#define	__SYSCALL_H__

// #define __NR_syscalls	    4

// #define SYS_WRITE_NUMBER    0 		// syscal numbers 
// #define SYS_MALLOC_NUMBER   1 	
// #define SYS_CLONE_NUMBER    2 	
// #define SYS_EXIT_NUMBER     3 	

#define SYS_CALL_GETPID     0
#define SYS_CALL_FORK       4
#define SYS_CALL_EXIT       5


#ifndef __ASSEMBLER__

#include "type.h"


typedef void (*syscall) ();


enum {
    SYS_GETPID,         // 0
    SYS_UART_RECV,      // 1
    SYS_UART_WRITE,     // 2
    SYS_EXEC,           // 3
    SYS_FORK,           // 4
    SYS_EXIT,           // 5
    SYS_MBOX,           // 6
    SYS_KILL_PID,       // 7
    SYS_SIGNAL,         // 8
    SYS_SIGKILL,        // 9
    SYS_SIGRETURN,      // 10
    NR_SYSCALLS         // 11
};


int32_t     sys_getpid();
uint32_t    sys_uart_read(byteptr_t buf, uint32_t size);
uint32_t    sys_uart_write(const byteptr_t buf, uint32_t size);
int32_t     sys_exec(const char *name, char *const argv[]);
int32_t     sys_fork();
void        sys_exit(int status);
int32_t     sys_mbox_call(byte_t ch, uint32_t *mbox);
void        sys_kill_pid(int32_t pid);

void        sys_signal();
void        sys_sigkill();
void        sys_sigreturn();

int32_t     call_get_pid(void);
int32_t     call_fork(void);


void * const syscall_table[NR_SYSCALLS] = {
    [SYS_GETPID] =      &sys_getpid,
    [SYS_UART_RECV] =   &sys_uart_read,
    [SYS_UART_WRITE] =  &sys_uart_write,
    [SYS_EXEC] =        &sys_exec,
    [SYS_FORK] =        &sys_fork,
    [SYS_EXIT] =        &sys_exit,
    [SYS_MBOX] =        &sys_mbox_call,
    [SYS_KILL_PID] =    &sys_kill_pid,
    [SYS_SIGNAL] =      &sys_signal,
    [SYS_SIGKILL] =     &sys_sigkill,
    [SYS_SIGRETURN] =   &sys_sigreturn,
};


#endif
#endif 