#include "signal.h"
#include "mm.h"
#include "sched.h"
#include "string.h"

extern void sigreturn(); // Defined in entry.S

void signal(int signum, void (*handler)())
{
    get_current()->sighand[signum] = handler;
}

void kill(int pid, int sig)
{
    struct task_struct *task = get_task(pid);
    task->sigpending |= 1 << (sig - 1); // Set the signal pending bit
}

void do_signal(trap_frame *regs)
{
    // Prevent nested signal handling
    if (get_current()->siglock)
        return;

    int signum = 1;
    while (get_current()->sigpending) {
        if (get_current()->sigpending & 0x1) {
            get_current()->sigpending &= ~(0x1);
            get_current()->siglock = 1;

            if (get_current()->sighand[signum] == 0) {
                kthread_exit(); // Default handler (exit the process)
                get_current()->siglock = 0;
                return; // Jump to the previous context (user program) after eret
            }

            // Save the sigframe
            memcpy(&get_current()->sigframe, regs, sizeof(trap_frame));
            get_current()->sig_stack = kmalloc(STACK_SIZE);
            map_pages((unsigned long)get_current()->pgd, 0xFFFFFFFF7000, 0x4000,
                      (unsigned long)VIRT_TO_PHYS(get_current()->sig_stack), 0);

            regs->x30 = (unsigned long)sigreturn;
            regs->spsr_el1 = 0x340;
            regs->elr_el1 = (unsigned long)get_current()->sighand[signum];
            regs->sp_el0 = (unsigned long)0xFFFFFFFFB000;
            return; // Jump to the signal handler after eret
        }
        signum++;
        get_current()->sigpending >>= 1;
    }
}
