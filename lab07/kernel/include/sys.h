#ifndef __SYS_H__
#define __SYS_H__

#define __NR_syscalls 20

#define SYS_GETPID_NUM      0   // syscall number
#define SYS_UARTREAD_NUM    1
#define SYS_UARTWRITE_NUM   2
#define SYS_EXEC_NUM        3
#define SYS_FORK_NUM        4
#define SYS_EXIT_NUM        5
#define SYS_MBOX_CALL_NUM   6
#define SYS_KILL_NUM        7


#define SYS_OPEN_NUM        11
#define SYS_CLOSE_NUM       12
#define SYS_WRITE_NUM       13
#define SYS_READ_NUM        14
#define SYS_MKDIR_NUM       15
#define SYS_MOUNT_NUM       16
#define SYS_CHDIR_NUM       17
#define SYS_LSEEK64_NUM     18
#define SYS_IOCTL_NUM       19

#ifndef __ASSEMBLER__

#include "type.h"

int sys_getpid();
size_t sys_uartread(char buf[], size_t size);
size_t sys_uartwrite(const char buf[], size_t size);
int sys_exec(const char *name, char *const argv[]);
int sys_fork();
void sys_exit(int status);
int sys_mbox_call(unsigned char ch, unsigned int *mbox);
void sys_kill(int pid);

// lab7
int sys_open(const char *pathname, int flags);
int sys_close(int fd);
long sys_write(int fd, const void *buf, unsigned long count);
long sys_read(int fd, void *buf, unsigned long count);

// you can ignore mode, since there is no access control
int sys_mkdir(const char *pathname, unsigned mode);

// you can ignore arguments other than target (where to mount) and filesystem (fs name)
int sys_mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);
int sys_chdir(const char *path);

long sys_lseek64(int fd, long offset, int whence);
long sys_ioctl(int fd, unsigned long request, unsigned long arg);

#endif

#endif
