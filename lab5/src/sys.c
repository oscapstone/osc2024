#include "../include/fork.h"
#include "../include/mini_uart.h"
#include "../include/sched.h"
#include "../include/mem_utils.h"
#include "../include/mailbox.h"
#include "../include/sys.h"

int sys_getpid(void)
{
    return current->pid;
}

uint32_t sys_uart_read(char *buff, uint32_t size)
{
    uint32_t i;
    for (i = 0; i < size; i++) {
        buff[i] = uart_recv();
        if (buff[i] == '\r')
            break;
    }
    return i;
}
uint32_t sys_uart_write(const char *buff, uint32_t size)
{
    uint32_t i;
    for (i = 0; i < size; i++) {
        uart_send(buff[i]);
        if (buff[i] == '\r') {
            uart_send('\n');
            break;
        }
    }
    return i;
}

int sys_exec(const char *name, char const *argv[])
{
    return 1;
}

int sys_fork(void)
{
    return copy_process(0, NULL, NULL);
}

void sys_exit(void)
{
    exit_process();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox)
{
    return mbox_call(ch, mbox);
}

void sys_kill(int pid)
{
    kill_process(pid);
}

void * const sys_call_table[] = {sys_getpid, sys_uart_read, sys_uart_write, sys_exec, sys_fork, sys_exit, sys_mbox_call, sys_kill};
