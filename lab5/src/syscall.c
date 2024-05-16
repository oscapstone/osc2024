#include "syscall.h"
#include "initrd.h"
#include "irq.h"
#include "mbox.h"
#include "mm.h"
#include "sched.h"
#include "signal.h"
#include "string.h"
#include "traps.h"
#include "uart.h"
#include "utils.h"

extern void child_ret_from_fork();

int sys_getpid()
{
    return get_current()->pid;
}

size_t sys_uart_read(char *buf, size_t size)
{
    int i = 0;
    while (i < size)
        buf[i++] = uart_getc();
    return i;
}

size_t sys_uart_write(const char *buf, size_t size)
{
    int i = 0;
    while (i < size)
        uart_putc(buf[i++]);
    return i;
}

int sys_exec(const char *name, char *const argv[])
{
    initrd_sys_exec(name);
    return 0;
}

int sys_fork(trap_frame *tf)
{
    disable_interrupt();
    struct task_struct *parent = get_current();
    struct task_struct *child = kthread_create(0);

    // Copy kernel stack and user stack
    memcpy(child->stack, parent->stack, STACK_SIZE);
    memcpy(child->user_stack, parent->user_stack, STACK_SIZE);

    // Copy signal handlers
    memcpy(child->sighand, parent->sighand, sizeof(parent->sighand));

    unsigned long sp_off = (unsigned long)tf - (unsigned long)parent->stack;
    trap_frame *child_trap_frame = (trap_frame *)(child->stack + sp_off);

    child->context.sp = (unsigned long)child_trap_frame;
    child->context.lr = (unsigned long)child_ret_from_fork;

    unsigned long sp_el0_off = tf->sp_el0 - (unsigned long)parent->user_stack;
    child_trap_frame->sp_el0 = (unsigned long)child->user_stack + sp_el0_off;
    child_trap_frame->x0 = 0;

    enable_interrupt();
    return child->pid;
}

void sys_exit()
{
    kthread_exit();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox)
{
    return mbox_call(ch, mbox);
}

void sys_kill(int pid)
{
    kthread_stop(pid);
}

void sys_signal(int signum, void (*handler)())
{
    signal(signum, handler);
}

void sys_sigkill(int pid, int sig)
{
    kill(pid, sig);
}

void sys_sigreturn(trap_frame *regs)
{
    // Restore the sigframe
    memcpy(regs, &get_current()->sigframe, sizeof(trap_frame));
    kfree(get_current()->sig_stack);
    get_current()->sighandling = 0;
    return; // Jump to the previous context (user program) after eret
}

/* System Call Test */

static int getpid()
{
    int pid = -1;
    asm volatile("mov x8, 0");
    asm volatile("svc 0");
    asm volatile("mov %0, x0" : "=r"(pid));
    return pid;
}

static int fork()
{
    int ret = -1;
    asm volatile("mov x8, 4");
    asm volatile("svc 0");
    asm volatile("mov %0, x0" : "=r"(ret));
    return ret;
}

static void exit()
{
    asm volatile("mov x8, 5");
    asm volatile("svc 0");
}

void fork_test()
{
    uart_puts("Fork Test (pid = ");
    uart_hex(getpid());
    uart_puts(")\n");
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) {
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        uart_puts("first child pid: ");
        uart_hex(getpid());
        uart_puts(", cnt: ");
        uart_hex(cnt);
        uart_puts(", ptr: ");
        uart_hex((unsigned long long)&cnt);
        uart_puts(", sp: ");
        uart_hex(cur_sp);
        uart_puts("\n");
        cnt++;
        if ((ret = fork()) != 0) {
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            uart_puts("first child pid: ");
            uart_hex(getpid());
            uart_puts(", cnt: ");
            uart_hex(cnt);
            uart_puts(", ptr: ");
            uart_hex((unsigned long long)&cnt);
            uart_puts(", sp: ");
            uart_hex(cur_sp);
            uart_puts("\n");
        } else {
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                uart_puts("second child pid: ");
                uart_hex(getpid());
                uart_puts(", cnt: ");
                uart_hex(cnt);
                uart_puts(", ptr: ");
                uart_hex((unsigned long long)&cnt);
                uart_puts(", sp: ");
                uart_hex(cur_sp);
                uart_puts("\n");
                for (int i = 0; i < 1000000; i++)
                    ;
                cnt++;
            }
        }
        exit();
    } else {
        uart_puts("parent here, pid ");
        uart_hex(getpid());
        uart_puts(", child ");
        uart_hex(ret);
        uart_puts("\n");
    }
    exit();
}

void run_fork_test()
{
    asm volatile("msr spsr_el1, %0" ::"r"(0x340));
    asm volatile("msr elr_el1, %0" ::"r"(fork_test));
    asm volatile("msr sp_el0, %0" ::"r"(get_current()->context.sp));
    asm volatile("mov sp, %0" ::"r"(get_current()->stack + STACK_SIZE));
    asm volatile("eret");
}