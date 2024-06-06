#include <kernel/signal.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <lib/string.h>

extern void sigreturn(); // Defined in entry.S

void signal(int signum, void (*handler)())
{
    get_current()->sighand[signum] = handler;
}

void sigkill(int pid, int sig)
{
    struct task_struct *task = get_task(pid);
    task->sigpending |= 1 << (sig - 1); // Set the signal pending bit
}

void do_signal(trap_frame *regs)
{
    // Prevent nested signal handling
    if (get_current()->sighandling)
        return;

    int signum = 1;
    while (get_current()->sigpending) {
        if (get_current()->sigpending & 0x1) {
            get_current()->sigpending &= ~(0x1); // make sure that we not handling the same signal again
            get_current()->sighandling = 1; 

            if (get_current()->sighand[signum] == 0) {
                kthread_exit(); // Default handler (exit the process)
                get_current()->sighandling = 0;
                return; // Jump to the previous context (user program) after eret
            }

            // Save the sigframe
            memcpy(&get_current()->sigframe, regs, sizeof(trap_frame));
            get_current()->sig_stack = kmalloc(STACK_SIZE);
            regs->x30 = (unsigned long)sigreturn;
            regs->spsr_el1 = 0x0;
            regs->elr_el1 = (unsigned long)get_current()->sighand[signum];
            regs->sp_el0 = (unsigned long)get_current()->sig_stack + STACK_SIZE;
            return; // Jump to the signal handler after eret
        }
        signum++;
        get_current()->sigpending >>= 1;
    }
}
