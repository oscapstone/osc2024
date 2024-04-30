#include "exception.h"

void disable_interrupt(){
    asm volatile("msr DAIFSet, 0xf;");
}

void enable_interrupt(){
    asm volatile("msr DAIFClr, 0xf;");
}

void exception_entry(){
    
    uint64_t spsr_el1;
	asm volatile("mrs %0, spsr_el1;" : "=r" (spsr_el1) :  : "memory");      
    
    uint64_t elr_el1;
	asm volatile("mrs %0, elr_el1;" : "=r" (elr_el1) :  : "memory");

    uint64_t esr_el1;
	asm volatile("mrs %0, esr_el1;" : "=r" (esr_el1) :  : "memory");

    print_str("\nspsr_el1: 0x");
    print_hex(spsr_el1);
    
    print_str("\nelr_el1: 0x");
    print_hex(elr_el1);

    print_str("\nesr_el1: 0x");
    print_hex(esr_el1);

}

void exec_in_el0(char* prog_head, uint32_t sp_loc){

    asm volatile(
        "mrs    x21, spsr_el1;"
        "mrs    x22, elr_el1;"
        "mov    x1, #0;"
        "msr    spsr_el1, x1;"
        "msr    elr_el1, %0;"
        "msr    sp_el0, %1;"
        "eret;"
        :: 
        "r"(prog_head),
        "r"(sp_loc)
    );
}

void el0_irq_entry(){

    if (*CORE0_IRQ_SOURCE & IRQ_SOURCE_CNTPNSIRQ){
        disable_core_timer();
        add_task(two_sec_timer_handler, TIMER_PRIO);
        pop_task();
        // enable_core_timer();
        // two_sec_timer_handler();
    }
}

void el1_irq_entry(){

    if ((*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT) && (*CORE0_IRQ_SOURCE & IRQ_SOURCE_GPU)){
        disable_uart_interrupt();
        add_task(async_uart_handler, UART_PRIO);
        pop_task();
        enable_uart_interrupt();
        // print_str("\nhere");
        // async_uart_handler();
    }else if (*CORE0_IRQ_SOURCE & IRQ_SOURCE_CNTPNSIRQ){
        disable_core_timer();

        if (!timer_empty()){
            add_task(core_timer_handler, TIMER_PRIO);
        }

        pop_task();
        
        enable_core_timer();
        // core_timer_handler();
    }

}