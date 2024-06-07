#include "signal.h"
#include "syscall.h"
#include "sched.h"
#include "memory.h"

extern thread_t *curr_thread;

/**
                                                                             (                                                                          )
    [Signal]       [UserProcess]                                             (                    [ Registered Signal Handler ]                         )     [UserProcess]
       |                  \                                                  (                   /                             \                        )    /
       |                   \                                                 (                  /                               \                       )   /
-------|--------------------[SystemCall]-------------------------------------(-----------------/---------------------------------\----------------------)--/---------------
       |                                \                     [Signal Check] (-> <is Registered>                                  \                     ) /
       V                                 \                   /      ^        (                 \                                   \                    )/
     [Job Pending]                        [Exception Handler]       |        (                  \ [ Default Signal Handler ]-------- [Exception Handler])
       |                                                            |        (                                                                          )
        -----------<trigger only if UserProcess do syscall>----------        (               CONTENT SWITCHING FOR SIGNAL HANDLER                       )

**/



void check_signal(trapframe_t *tpf)
{
    if(curr_thread->signal.lock) return;
    lock();
    // Prevent nested running signal handler. No need to handle
    curr_thread->signal.lock = 1;
    unlock();
    for (int i = 0; i < SIGNAL_MAX; i++)
    {
        store_context(&curr_thread->signal.saved_context);
        if(curr_thread->signal.pending[i]>0)
        {
            lock();
            curr_thread->signal.pending[i]--;
            unlock();
            run_signal(tpf, i);
        }
    }
    lock();
    curr_thread->signal.lock = 0;
    unlock();
}

void run_signal(trapframe_t *tpf, int signal)
{
    lock();
    curr_thread->signal.curr_handler = curr_thread->signal.handler_table[signal];
    unlock();

    // run default handler in kernel
    if (curr_thread->signal.curr_handler == signal_default_handler)
    {
        signal_default_handler();
        return;
    }

    // run registered handler in userspace
    // lock();
    curr_thread->signal.stack_base = kmalloc(USTACK_SIZE);
    asm("msr tpidr_el1, %0\n\t" /* Hold the \"kernel(el1)\" thread structure information */
        "msr elr_el1, %1\n\t"
        "msr sp_el0, %2\n\t"
        "msr spsr_el1, %3\n\t"
        "eret\n\t" 
        :
        :"r"(&curr_thread->context),
        "r"(signal_handler_wrapper),
        "r"(curr_thread->signal.stack_base + USTACK_SIZE),
        "r"(tpf->spsr_el1));

}

void signal_handler_wrapper()
{
    // unlock();
    (curr_thread->signal.curr_handler)();
    // system call sigreturn
    asm("mov x8,10\n\t"
        "svc 0\n\t");
}

void signal_default_handler()
{
    kill(0,curr_thread->pid);
}