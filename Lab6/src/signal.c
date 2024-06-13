#include "signal.h"
#include "sched.h"
#include "syscall.h"
#include "memory.h"
#include "mmu.h"


void check_signal(trapframe_t *tp)
{
    lock();
    if (curr_thread->signal_is_checking)
    {
        unlock();
        return;
    }
    curr_thread->signal_is_checking = 1;
    unlock();
    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
        store_context(&curr_thread->signal_saved_context);
        if (curr_thread->sigcount[i] > 0)
        {
            lock();
            curr_thread->sigcount[i]--;
            unlock();
            run_signal(tp, i);
        }
    }
    lock();
    curr_thread->signal_is_checking = 0;
    unlock();
}

void run_signal(trapframe_t *tp, int signal)
{
    curr_thread->curr_signal_handler = curr_thread->signal_handler[signal];

    if (curr_thread->curr_signal_handler == signal_default_handler)
    {
        signal_default_handler();
        return;
    }

    // char *temp_signal_userstack = kmalloc(USTACK_SIZE);
    // registered handler go to el0
    asm("msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t"
        "msr spsr_el1, %2\n\t"
	    "mov x0, %3\n\t"
        "eret\n\t"
        :: "r"(USER_SIGNAL_WRAPPER_VA + ((size_t)signal_handler_wrapper % 0x1000)),
           "r"(tp->sp_el0),
           "r"(tp->spsr_el1),
           "r"(curr_thread->curr_signal_handler));
}

// free signal stack
void signal_handler_wrapper()
{
    // system call sigreturn
    asm("blr x0\n\t"
        "mov x8,50\n\t"
        "svc 0\n\t");
}

void signal_default_handler()
{
    kill(0, curr_thread->pid);
}