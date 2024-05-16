#include "uart.h"
#include "schedule.h"
#include "allocator.h"
#include "signal.h"

void default_SIGKILL_handler()
{
    task_exit();
}

void do_signal(struct ucontext *sigframe, void (*signal_handler)(void))
{
    task_struct *cur = get_current_task();
    cur->signal_stack = kmalloc(2 * 4096); // malloc a stack's space to deal with signal

    unsigned long long *sp_ptr = cur->signal_stack;

    for (int i = 0; i < 34; i++)
        *(sp_ptr - i) = *((unsigned long long *)sigframe + i); // push sigframe to signal stack

    sp_ptr -= 34;

    asm volatile(
        "msr sp_el0, %0\n"
        "msr elr_el1, %1\n"
        "mov x10, 0\n"
        "msr spsr_el1, x10\n"
        "mov lr, %2\n" // set lr to sigreturn to restore context
        "mov sp, %3\n"
        "eret\n"
        :
        : "r"(sp_ptr), "r"(signal_handler), "r"(sigreturn), "r"(cur->kstack));
}
