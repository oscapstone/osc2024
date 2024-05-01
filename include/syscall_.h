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
void exit(trapframe_t *tpf);
uint32_t sys_mbox_call(trapframe_t *tpf, uint8_t ch, uint32_t *mbox);
void kill(uint32_t pid);
#endif