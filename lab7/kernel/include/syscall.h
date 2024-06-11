#ifndef __SYSCALL_H
#define __SYSCALL_H

#define SYS_GET_PID 0
#define SYS_UART_READ 1
#define SYS_UART_WRITE 2
#define SYS_EXEC 3
#define SYS_FORK 4
#define SYS_EXIT 5
#define SYS_MBOX_CALL 6
#define SYS_KILL 7
#define SYS_SIGNAL 8
#define SYS_SIGNAL_KILL 9
#define SYS_MMAP 10

#define SYS_OPEN 11
#define SYS_CLOSE 12
#define SYS_WRITE 13
#define SYS_READ 14
#define SYS_MKDIR 15
#define SYS_MOUNT 16
#define SYS_CHDIR 17
#define SYS_LSEEK64 18

#define SYS_SIGRETURN 50

#define SEEK_SET 0

struct ucontext
{
    unsigned long long x[31]; // general register from x0 ~ x30
    unsigned long long sp_el0;
    unsigned long long elr_el1;
    unsigned long long spsr_el1;
};

int getpid();
int uartread(char buf[], int size);
int uartwrite(const char buf[], int size);
int exec(const char *name, char *const argv[]);
int fork();
void exit(int status);
int mbox_call(unsigned char ch, unsigned int *mbox);
void kill(int pid);
void signal(int SIGNAL, void (*handler)());
void signal_kill(int pid, int SIGNAL);
void *mmap(void *addr, unsigned long long len, int prot, int flags, int fd, int file_offset);

int open(const char *pathname, int flags);
int close(int fd);
long write(int fd, const void *buf, unsigned long count);
long read(int fd, void *buf, unsigned long count);
int mkdir(const char *pathname, unsigned mode);
int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);
int chdir(const char *path);
long lseek64(int fd, long offset, int whence);

void sigreturn();

#endif