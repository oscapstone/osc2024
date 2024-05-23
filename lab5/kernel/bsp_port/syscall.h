#ifndef __SYS_H__
#define __SYS_H__

#define __NR_syscalls 8

#define SYS_GETPID_NUM 0  // syscall number
#define SYS_UARTREAD_NUM 1
#define SYS_UARTWRITE_NUM 2
#define SYS_EXEC_NUM 3
#define SYS_FORK_NUM 4
#define SYS_EXIT_NUM 5
#define SYS_MBOX_CALL_NUM 6
#define SYS_KILL_NUM 7

#ifndef __ASSEMBLER__

#include <lib/stddef.h>

int getpid();
size_t uartread(char buf[], size_t size);
size_t uartwrite(const char buf[], size_t size);
int exec(const char *name, char *const argv[]);
int fork();
void exit(int status);
int mbox_call(unsigned char ch, unsigned int *mbox);
void kill(int pid);

#endif
#endif
