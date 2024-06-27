#include <stdint.h>
#include "alloc.h"
#include "signal.h"
#include "syscall.h"
#include "string.h"
#include "thread.h"
#include "mmu.h"

void check_and_run_signal() {
    thread_t* cur_thread = get_current_thread();
    // if(cur_thread -> tid == 3) {
    //     uart_send_string("[signal] chcecking for tid: ");
    //     uart_hex(cur_thread->tid);
    //     uart_send_string("\n");
    //     uart_send_string("[signal] signal 9: ");
    //     uart_hex(cur_thread->waiting_signal[9]);
    //     uart_send_string("\n");
    // }
    for(int i=0;i<=SIGNAL_NUM;i++) {
        if(cur_thread->waiting_signal[i]) {
            cur_thread->waiting_signal[i] = 0;
            exec_signal(cur_thread, i);
        }
    }
}

void exec_signal(thread_t* t, int signal) {
    if(t -> is_processing_signal) return; // don't need to handle nested signal
    t -> is_processing_signal = 1;
    uart_send_string("enter signal handler\n");
    // default signal handler can be run in kernel mode
    if(!t->signal_handler[signal]) {
        default_signal_handler();
        return;
    }
    // copy current callee register to thread's callee register
    memcpy((void*)&t->signal_regs, (void*)&t->callee_reg, sizeof(callee_reg_t));

    // set new user stack for signal handler
    t -> callee_reg.sp = (uint64_t)kmalloc(T_STACK_SIZE) + T_STACK_SIZE;
    t -> callee_reg.fp = t -> callee_reg.sp;
    t -> callee_reg.lr = handler_warper;

    unsigned long spsr_el1 = 0x0; // run in el0 and enable all interrupt (DAIF)
    unsigned long elr_el1 = (unsigned long)handler_warper;
    unsigned long user_sp = t -> callee_reg.sp;
    // switch to user mode
    asm volatile("msr spsr_el1, %0" : : "r" (spsr_el1));
    asm volatile("msr elr_el1, %0" : : "r" (elr_el1));
    asm volatile("msr sp_el0, %0" : : "r" (user_sp));
    asm volatile("mov x0, %0" : : "r" (t->signal_handler[signal]));
    asm volatile("eret"); // jump to user program
}

void handler_warper(void (*handler)()) {
    handler();
    sys_sigreturn();
}

void default_signal_handler() {
    uart_send_string("Default signal handler\n");
    uart_send_string("current thread tid: ");
    uart_hex(get_current_thread()->tid);
    uart_send_string("\n");
    kill(get_current_thread()->tid);
}