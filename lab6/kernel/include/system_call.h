#ifndef SYSTEM_CALL_H
#define SYSTEM_CALL_H

#include "exception.h"
#include <stddef.h>

typedef void (*handler) (void);

int do_getpid();
size_t do_uart_read(char buf[], size_t size);
size_t do_uart_write(char buf[], size_t size);
int do_exec(char* name, char *argv[]);
int do_fork(trapframe_t*);
void do_exit();
int do_mbox_call(unsigned char ch, unsigned int *mbox);
void do_kill(int pid);
void do_signal(int, handler);
void do_sigkill(int, int);

void from_el1_to_fork_test();
void simple_fork_test();

// for debug purpose
void get_sp();
void output_sp(void*);

#endif
