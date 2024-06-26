#include "sys.h"
#include "cpio.h"
#include "fork.h"
#include "mailbox.h"
#include "memory.h"
#include "mini_uart.h"
#include "sched.h"
#include "signal.h"

extern volatile unsigned int __attribute__((aligned(16))) mbox_buf[36];

int sys_getpid(void)
{
    return current_task->pid;
}

size_t sys_uart_read(char buf[], size_t size)
{
    size_t i;
    for (i = 0; i < size; i++)
        buf[i] = uart_recv();

    return i;
}


size_t sys_uart_write(const char buf[], size_t size)
{
    size_t i;
    for (i = 0; i < size; i++) {
        if (buf[i] == '\n')
            uart_send('\r');
        uart_send(buf[i]);
    }
    return i;
}

int sys_exec(const char* name, char const* argv[])
{
    return cpio_load((char*)name);
}

int sys_fork(void)
{
    return copy_process(0, NULL, NULL, NULL);
}

void sys_exit(void)
{
    exit_process();
}


int sys_mbox_call(unsigned char ch, unsigned int* mbox)
{
    size_t usr_mbox_sz = mbox[0];
    memcpy((void*)mbox_buf, mbox, usr_mbox_sz);
    // unsigned long offset =
    //     (unsigned long)mbox - ((unsigned long)mbox & PAGE_MASK);
    // unsigned long* entry = find_page_entry(current_task, (unsigned
    // long)mbox); unsigned int* mbox_map = (unsigned int*)(*entry + offset);
    // int res = mailbox_call(ch, mbox_map);
    int res = mailbox_call(ch, (void*)mbox_buf);
    memcpy(mbox, (void*)mbox_buf, usr_mbox_sz);
    return res;
}


void sys_kill(int pid)
{
    struct task_struct* target = find_task(pid);
    kill_task(target);
}


void sys_signal(int SIGNAL, void (*handler)())
{
    reg_sig_handler(current_task, SIGNAL, handler);
}


void sys_sigkill(int pid, int SIGNAL)
{
    struct task_struct* task = find_task(pid);
    if (!task)
        return;
    recv_sig(task, SIGNAL);
}

void sys_sig_return(void)
{
    do_sig_return();
}

void* const sys_call_table[] = {[SYS_GET_PID_NUMBER] = sys_getpid,
                                [SYS_UART_READ_NUMBER] = sys_uart_read,
                                [SYS_UART_WRITE_NUMBER] = sys_uart_write,
                                [SYS_EXEC_NUMBER] = sys_exec,
                                [SYS_FORK_NUMBER] = sys_fork,
                                [SYS_EXIT_NUMBER] = sys_exit,
                                [SYS_MBOX_CALL_NUMBER] = sys_mbox_call,
                                [SYS_KILL_NUMBER] = sys_kill,
                                [SYS_SIGNAL_NUMBER] = sys_signal,
                                [SYS_SIGKILL_NUMBER] = sys_sigkill,
                                [SYS_SIG_RETURN_NUMBER] = sys_sig_return};
