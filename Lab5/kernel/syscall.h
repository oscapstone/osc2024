#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <stdint.h>

#define SYS_GETPID      0
#define SYS_UART_READ   1
#define SYS_UART_WRITE  2
#define SYS_EXEC        3
#define SYS_FORK        4
#define SYS_EXIT        5
#define SYS_MAILBOX     6

uint64_t getpid(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) ;

#endif