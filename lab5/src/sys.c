#include "../include/fork.h"
#include "../include/mini_uart.h"
#include "../include/sched.h"
#include "../include/mem_utils.h"

void sys_write(char *buf)
{
    printf(buf);
}

int sys_clone(unsigned long stack)
{
    //
}

unsigned long sys_malloc()
{
    unsigned long addr = page_frame_allocate(4);
    if (!addr)
        return -1;
    return addr;
}

void sys_exit()
{
    exit_process();
}

void * const sys_call_table[] = {sys_write, sys_malloc, sys_clone, sys_exit};