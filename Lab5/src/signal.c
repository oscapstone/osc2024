#include "signal.h"
#include "syscall.h"
#include "schedule.h"
#include "memory.h"

extern thread_t *curr_thread;

void check_signal(trapframe_t *tp)
{
    if (curr_thread->signal_inProcess)
        return;
    disable_irq();
    curr_thread->signal_inProcess = 1;
    enable_irq();
    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
        store_context(&curr_thread->signal_savedContext);
        if(curr_thread->sigcount[i]>0){
            disable_irq();
            curr_thread->sigcount[i]--;
            enable_irq();
            run_signal(tp, i);
        }
    }
    disable_irq();
    curr_thread->signal_inProcess = 0;
    enable_irq();
}

void run_signal(trapframe_t *tp, int signal){
    curr_thread->curr_signal_handler = curr_thread->signal_handler[signal];

    if(curr_thread->curr_signal_handler == signal_default_handler){
        signal_default_handler();
        return;
    }

    char *temp_signal_userstack = kmalloc(USTACK_SIZE);
    //registered handler go to el0
    asm("msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t"
        "msr spsr_el1, %2\n\t"
        "eret\n\t" ::"r"(signal_handler_wrapper),
        "r"(temp_signal_userstack + USTACK_SIZE),
        "r"(tp->spsr_el1));
}

// free signal stack
void signal_handler_wrapper()
{
    (curr_thread->curr_signal_handler)();
    // system call sigreturn
    asm("mov x8,10\n\t"
        "svc 0\n\t");
}

void signal_default_handler()
{
    kill(0,curr_thread->pid);
}