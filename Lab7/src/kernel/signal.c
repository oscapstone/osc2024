#include "kernel/signal.h"

void signal_default_handler(){
    kill(current_task->pid);
}

void check_signal(trap_frame_t *tf){
    lock();
    //uart_itoa(current_task->pid);
    //uart_puts(" is checking signal\n");
    // to avoid reentrant
    if(current_task->signal_is_checking){
        unlock();
        return;
    }
    current_task->signal_is_checking = 1;
    unlock();
    
    for(int i = 0; i <= NR_SIGNALS; i++){
        // save the context of the process, used for sigret
        store_context(&current_task->signal_saved_context);
        if(current_task->sigcount[i] > 0){
            lock();
            //uart_puts("Signal ");
            current_task->sigcount[i]--;
            unlock();

            run_signal(tf, i);
            //break;
        }
    }

    lock();
    current_task->signal_is_checking = 0;
    unlock();
}

void run_signal(trap_frame_t *tf, int signal){
    current_task->cur_signal_handler = current_task->signal_handler[signal];
    if(current_task->cur_signal_handler == signal_default_handler){
        signal_default_handler();
        return;
    }

    char *sig_stk = pool_alloc(THREAD_STK_SIZE);
    uart_b2x_64((unsigned long long)tf->spsr_el1);

    asm("msr elr_el1, %[var1];"
        "msr sp_el0, %[var2];"
        "msr spsr_el1, %[var3];"
        "eret\n\t"
        :
        :[var1] "r" (signal_handler_wrapper), [var2] "r" (sig_stk + THREAD_STK_SIZE), [var3] "r" (tf->spsr_el1));
}

void signal_handler_wrapper(){
    // execute the signal handler(if it's not default)
    current_task->cur_signal_handler();
    
    asm("mov x8, #64;"
        "svc #0;");
}