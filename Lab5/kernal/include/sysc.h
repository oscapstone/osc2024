#include"irq.h"

int sys_getpid(trapframe_t* tf);
unsigned long sys_uart_read(trapframe_t* tf);
unsigned long sys_uart_write(trapframe_t* tf);
int sys_mbox_call(trapframe_t *tpf);
void sys_kill(trapframe_t *tpf);