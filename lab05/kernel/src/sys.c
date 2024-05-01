#include "sys.h"
#include "schedule.h"
#include "mini_uart.h"

extern struct task_struct *current;

int sys_getpid()
{
    return current->pid;
}

size_t sys_uartread(char buf[], size_t size)
{
    for(size_t i=0; i<size; i++){
        buf[i] = uart_recv();
    }
    return size;
}

size_t sys_uartwrite(const char buf[], size_t size)
{
    for(size_t i=0; i<size; i++){
        uart_send(buf[i]);
    }
    return size;
}

int sys_exec(const char *name, char *const argv[]) // [TODO]
{
    return -1;
}

int sys_fork() // [TODO]
{
    return -1;
}

void sys_exit(int status) // [TODO]
{
    return;
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox) // [TODO]
{
    return 0;
}

void sys_kill(int pid) // [TODO]
{

}

void * const sys_call_table[] = {sys_getpid, sys_uartread, sys_uartwrite, sys_exec, sys_fork, sys_exit, sys_mbox_call, sys_kill};