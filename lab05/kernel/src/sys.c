#include "sys.h"
#include "schedule.h"
#include "fork.h"
#include "mini_uart.h"
#include "io.h"
#include "alloc.h"

extern struct task_struct *current;
extern void memzero_asm(unsigned long src, unsigned long n);

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
    unsigned long stack = (unsigned long)balloc(4*THREAD_SIZE);
    memzero_asm(stack, 4*THREAD_SIZE);
    return copy_process(0, 0, 0, stack);
}

void sys_exit(int status) // [TODO]
{
    exit_process();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox) // [TODO]
{
    return 0;
}

void sys_kill(int pid) // [TODO]
{
    struct task_struct* p;
    for(int i=0; i<NR_TASKS; i++){
        if(task[i] == NULL) continue; // (task[i] == NULL) means no more tasks
        p = task[i];
        if(p->pid == pid){
            preempt_disable();
            printf("\r\nKilling process: "); printf_int(pid);
            p->state = TASK_ZOMBIE;
            preempt_enable();
            return;
        }
    }
    printf("\r\nProcess not found: "); printf_int(pid);
    return;
}

void * const sys_call_table[] = {sys_getpid, sys_uartread, sys_uartwrite, sys_exec, sys_fork, sys_exit, sys_mbox_call, sys_kill};