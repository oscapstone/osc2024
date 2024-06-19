#ifndef SYS_H
#define SYS_H


#define NR_SYSCALLS 11

#define SYS_GET_PID_NUMBER    0
#define SYS_UART_READ_NUMBER  1
#define SYS_UART_WRITE_NUMBER 2
#define SYS_EXEC_NUMBER       3
#define SYS_FORK_NUMBER       4
#define SYS_EXIT_NUMBER       5
#define SYS_MBOX_CALL_NUMBER  6
#define SYS_KILL_NUMBER       7
#define SYS_SIGNAL_NUMBER     8
#define SYS_SIGKILL_NUMBER    9
#define SYS_SIG_RETURN_NUMBER 10

#ifndef __ASSEMBLER__

#include "def.h"
int sys_getpid(void);
size_t sys_uart_read(char buf[], size_t size);
size_t sys_uart_write(const char buf[], size_t size);
int sys_exec(const char* name, char const* argv[]);
int sys_fork(void);
void sys_exit(void);
int sys_mbox_call(unsigned char ch, unsigned int* mbox);
void sys_kill(int pid);
void sys_signal(int SIGNAL, void (*handler)());
void sys_sigkill(int pid, int SIGNAL);
void sys_sig_return(void);

extern int getpid(void);
extern size_t uart_read(char buf[], size_t size);
extern size_t uart_write(const char buf[], size_t size);
extern int exec(const char* name, char const* argv[]);
extern int fork(void);
extern void exit(void);
extern int mbox_call(unsigned char ch, unsigned int* mbox);
extern void kill(int pid);

#endif


#endif /* SYS_H */
