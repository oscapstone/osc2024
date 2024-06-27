#ifndef SYS_H
#define SYS_H

#define __NR_syscalls             8

#define SYS_GET_PID_NUMBER        0         // syscal numbers
#define SYS_UART_READ_NUMBER      1
#define SYS_UART_WRITE_NUMBER     2
#define SYS_EXEC_NUMBER           3
#define SYS_FORK_NUMBER           4
#define SYS_EXIT_NUMBER           5
#define SYS_MBOX_CALL_NUMBER      6
#define SYS_KILL_NUMBER           7

#ifndef __ASSEMBLER__

int sys_getpid(void);
uint32_t sys_uart_read(char *buff, uint32_t size);
uint32_t sys_uart_write(const char *buff, uint32_t size);
int sys_exec(const char *name, char const *argv[]);
int sys_fork(void);
void sys_exit(void);
int sys_mbox_call(unsigned char ch, unsigned int *mbox);
void sys_kill(int pid);



extern int getpid(void);
extern uint32_t uart_read(char *buff, uint32_t size);
extern uint32_t uart_write(const char *buff, uint32_t size);
extern int exec(const char *name, char const *argv[]);
extern int fork(void);
extern void exit(void);
extern int mailbox_call(unsigned char ch, unsigned int *mbox);
extern void kill(int pid);

#endif
#endif  /* SYS_H */