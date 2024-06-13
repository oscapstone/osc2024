#include "signal.h"
#include "memory.h"
#include "sched.h"
#include "syscall.h"

extern thread_t *current;

void default_signal_handler() { kill(0, current->pid); }

void handler_wrapper()
{
    (current->curr_signal_handler)();

    asm("mov x8, 50\n\t"
        "svc 0\n\t");
}

void check_signal(trapframe_t *tf)
{
    if (current->signal_processing)
        return;

    lock();
    current->signal_processing = 1;
    unlock();

    for (int i = 0; i < SIGNAL_NUM; i++) {
        store_context(&current->signal_context);
        if (current->signal_waiting[i] > 0) {
            lock();
            current->signal_waiting[i]--;
            unlock();
            run_signal(tf, i);
        }
    }

    lock();
    current->signal_processing = 0;
    unlock();
}

void run_signal(trapframe_t *tf, int signal)
{
    current->curr_signal_handler = current->signal_handler[signal];

    if (current->curr_signal_handler == default_signal_handler) {
        default_signal_handler();
        return;
    }

    char *new_signal_ustack = kmalloc(USTACK_SIZE);
    asm("msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t"
        "msr spsr_el1, %2\n\t"
        "eret\n\t" ::"r"(handler_wrapper),
        "r"(new_signal_ustack + USTACK_SIZE), "r"(tf->spsr_el1));
}