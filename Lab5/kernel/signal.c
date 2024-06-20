#include "signal.h"

extern thread_t* cur_thread;

void check_signal(trap_frame_t* tpf){

    // store_ctx(&cur_thread->signal_ctx);
    
    for (int i = 0; i < MAX_SIGNAL; i++){

        if (cur_thread->signal_count[i] > 0){

            // print_str("\nPID: ");
            // print_dec(cur_thread->id);
            // print_str("\nSignal Count: ");
            // print_dec(cur_thread->signal_count[i]);

            disable_interrupt();

            cur_thread->signal_count[i]--;

            enable_interrupt();

            handle_signal(tpf, i);  
        }
    }

}

void handle_signal(trap_frame_t* tpf, int SIGNAL){
    signal_handler_t exec_handler = cur_thread->signal_handler[SIGNAL];

    if (exec_handler == 0){
        // print_str("\nBBBB");
        default_signal_handler();
        return;
    }

    cur_thread->cur_signal_handler = exec_handler;
    cur_thread->signal_handler_sp = (uint8_t*)malloc(USTACK_SIZE);
    // print_str("\nSignal_handler_sp: ");
    // print_hex(cur_thread->signal_handler_sp);

    cur_thread->sig_ctx.x30 = tpf->x30;
    cur_thread->sig_ctx.sp_el0 = tpf->sp_el0;
    cur_thread->sig_ctx.elr_el1 = tpf->elr_el1;
    cur_thread->sig_ctx.spsr_el1 = tpf->spsr_el1;

    tpf->sp_el0 = cur_thread->signal_handler_sp + USTACK_SIZE;
    tpf->elr_el1 = exec_handler;
    tpf->x30 = (uint64_t)user_signal_return;

}

void signal_handler_run(){
    cur_thread->cur_signal_handler();
    user_signal_return();
    print_str("\nBBBB");
}

void user_signal_return(){

    asm volatile(
        "mov    x8, 10;"
        "svc    0;"
    );
}

void default_signal_handler(){
    sys_kill(cur_thread->id);
    // print_str("cur_thread status: ");
    // print_dec(cur_thread->status);
    // enable_interrupt();
    schedule();
}