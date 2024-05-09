#include "syscall.h"
#include "sched.h"
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
    return 0;
}

size_t sys_uart_write(const char *buf, size_t size)
{
    return 0;
}

int sys_exec(const char *name, char *const argv[])
{
    return 0;
}

int get_link_register()
{
    unsigned long lr;
    asm volatile("mov %0, lr" : "=r"(lr));
    return lr;
}

int sys_fork(trap_frame *tf)
{
    struct task_struct *parent = get_current();
    struct task_struct *child = kthread_create(0);

    // Copy registers
    // memcpy(&child->context, &parent->context, sizeof(struct thread_struct));

    // Copy kernel stack and user stack
    memcpy(child->stack, parent->stack, STACK_SIZE);
    memcpy(child->user_stack, parent->user_stack, STACK_SIZE);

    unsigned long offset = (unsigned long)tf - (unsigned long)parent->stack;
    trap_frame *child_trap_frame = (trap_frame *)(child->stack + offset);
    child->context.sp = (unsigned long)child_trap_frame;
    child->context.lr = (unsigned long)child_ret_from_fork; // Redirect to eret
    child_trap_frame->x0 = 0;

    if (parent->pid == get_current()->pid) {
        return child->pid;
    } else {
        return 0;
    }
}

void sys_exit()
{
    kthread_exit();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox)
{
    return 0;
}

void sys_kill(int pid)
{
}

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
    from_el1_to_el0();

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
                for (int i = 0; i < 500000000; i++)
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