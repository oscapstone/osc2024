#include "signal.h"

void check_signal(trapframe_t *tpf) {
    lock();
    // detect if its nested checking signal
    if (curr_thread->signal_is_checking) {
        unlock();
        return;
    }
    // prevent nested running signal handler
    curr_thread->signal_is_checking = 1;
    unlock();

    for (int i = 0; i <= SIGNAL_MAX; ++i) {
        // save original context before running handler
        store_context(&curr_thread->signal_saved_context);
        if (curr_thread->sigcount[i] > 0) {
            lock();
            curr_thread->sigcount[i]--;
            unlock();
            run_signal(tpf, i);
        }
    }
    lock();
    // checking finished
    curr_thread->signal_is_checking = 0;
    unlock();
}

void run_signal(trapframe_t *tpf, int signal) {
    // set now callback according to signal index
    lock();
    curr_thread->curr_signal_handler = curr_thread->signal_handler[signal];
    unlock();
    // run default handler in kernel
    if (curr_thread->curr_signal_handler == signal_default_handler) {
        //kill
        signal_default_handler();
        return;
    }
    // kernel malloc signal handler stack
    char *temp_signal_userstack = malloc(USTACK_SIZE);
    // set elr_el1 callback
    __asm__ __volatile__(
        "msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t"
        "msr spsr_el1, %2\n\t"
        "eret\n\t" ::"r"(signal_handler_wrapper),
        "r"(temp_signal_userstack + USTACK_SIZE),
        "r"(tpf->spsr_el1));
}

void signal_handler_wrapper() {
    // run callback
    curr_thread->curr_signal_handler();
    // system call sigreturn
    // pre set syscall number
    // run signal return to restore orignal context
    asm("mov x8,50\n\t"
        "svc 0\n\t");
}

void signal_default_handler() {

    kill(0, curr_thread->pid);
}