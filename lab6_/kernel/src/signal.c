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
    if(curr_thread->signal_is_checking) return;
    lock();
    // Prevent nested running signal handler. No need to handle
    curr_thread->signal_is_checking = 1;
    unlock();
    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
        store_context(&curr_thread->signal_saved_context);
        if(curr_thread->sigcount[i]>0)
        {
            lock();
            curr_thread->sigcount[i]--;
            unlock();
            run_signal(tpf, i);
        }
    }
    lock();
    curr_thread->signal_is_checking = 0;
    unlock();
}

void run_signal(trapframe_t *tpf, int signal)
{
    curr_thread->curr_signal_handler = curr_thread->signal_handler[signal];

    // run default handler in kernel
    if (curr_thread->curr_signal_handler == signal_default_handler)
    {
        signal_default_handler();
        return;
    }

    // run registered handler in userspace
    char *new_signal_userstack = kmalloc(USTACK_SIZE);
    asm("msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t"
        "msr spsr_el1, %2\n\t"
        "eret\n\t" 
        :
        :"r"(signal_handler_wrapper),
         "r"(new_signal_userstack + USTACK_SIZE),
         "r"(tpf->spsr_el1));

}

void signal_handler_wrapper()
{
    (curr_thread->curr_signal_handler)();

    // system call sigreturn
    asm(
        "mov x8,50\n\t"
        "svc 0\n\t"
    );
}

void signal_default_handler()
{
    trapframe_t tpf;
    tpf.x0 = curr_thread->pid;
    kill(&tpf);
}
