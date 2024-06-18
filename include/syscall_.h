#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "types.h"

#define SYSCALL_GET_PID 0
#define SYSCALL_UARTREAD 1
#define SYSCALL_UARTWRITE 2
#define SYSCALL_EXEC 3
#define SYSCALL_FORK 4
#define SYSCALL_EXIT 5
#define SYSCALL_MBOX 6
#define SYSCALL_KILL 7
#define SYSCALL_SIG 8
#define SYSCALL_SIGKILL 9
#define SYSCALL_MMAP 10
#define SYSCALL_OPEN 11
#define SYSCALL_CLOSE 12
#define SYSCALL_WRITE 13
#define SYSCALL_READ 14
#define SYSCALL_MKDIR 15
#define SYSCALL_MOUNT 16
#define SYSCALL_CHDIR 17
#define SYSCALL_LSEEK64 18
#define SYSCALL_SYNCFS 20
#define SIG_RETURN 32
#define SIG_NUM 32

typedef void (*sig_handler_func)();

typedef struct sig_handler_t {
  uint8_t registered;
  uint32_t sig_cnt;
  sig_handler_func func;
} sig_handler_t;

typedef struct trapframe_t {
  uint64_t x[31];  // general registers from x0 ~ x30
  uint64_t sp_el0;
  uint64_t spsr_el1;
  uint64_t elr_el1;
} trapframe_t;

uint64_t sys_getpid(trapframe_t *tpf);
uint32_t sys_uartread(trapframe_t *tpf, char buf[], uint32_t size);
uint32_t sys_uartwrite(trapframe_t *tpf, const char buf[], uint32_t size);
uint32_t exec(trapframe_t *tpf, const char *name, char *const argv[]);
uint32_t fork();
void exit();
uint32_t sys_mbox_call(trapframe_t *tpf, uint8_t ch, uint32_t *mbox);
void kill(uint32_t pid);
void signal(uint32_t SIGNAL, sig_handler_func handler);
void sig_kill(uint32_t pid, uint32_t SIGNAL);
void sys_mmap(trapframe_t *tpf);
void sys_open(trapframe_t *tpf);
void sys_close(trapframe_t *tpf);
void sys_write(trapframe_t *tpf);
void sys_read(trapframe_t *tpf);
void sys_mkdir(trapframe_t *tpf);
void sys_mount(trapframe_t *tpf);
void sys_chdir(trapframe_t *tpf);
void sys_lseek64(trapframe_t *tpf);
void sys_syncfs(trapframe_t *tpf);
void check_signal();
void sig_kill_default_handler();
void sig_return();
#endif