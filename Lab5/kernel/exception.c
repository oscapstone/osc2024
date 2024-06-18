#include "exception.h"

// int lock_count = 0;

extern thread_t* cur_thread;

void disable_interrupt(){

    // if (lock_count == 0)
    asm volatile("msr DAIFSet, 0xf;");   
    
    // lock_count++;
}

void enable_interrupt(){

    // if (lock_count > 0)
    //     lock_count--;
    
    // if (lock_count == 0)
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
        "msr    spsr_el1, xzr;"
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

        // if (cur_thread->id == 2){
        //     print_str("\nDAIF: ");
        //     print_hex(read_DAIF());
        //     print_str("\nTimer CTL: ");
        //     print_hex(read_core_timer_enable());
        //     print_str("\nTimer expire time: ");
        //     print_hex(read_core_timer_expire());
        //     print_str("\nFreq >> 5: ");
        //     print_hex(get_core_freq() >> 5);
        // }
        add_task(core_timer_handler, TIMER_PRIO);
        pop_task();
        enable_core_timer();
        // two_sec_timer_handler();
    }
}

void el1_irq_entry(){

    if ((*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT) && (*CORE0_IRQ_SOURCE & IRQ_SOURCE_GPU)){
        disable_uart_interrupt();
        add_task(async_uart_handler, UART_PRIO);
        pop_task();
        enable_uart_interrupt();

        // async_uart_handler();
    }else if (*CORE0_IRQ_SOURCE & IRQ_SOURCE_CNTPNSIRQ){
        disable_core_timer();

        // if (!timer_empty()){
        //     add_task(core_timer_handler, TIMER_PRIO);
        // }
        // enable_core_timer();
        // enable_interrupt();
        add_task(core_timer_handler, TIMER_PRIO);
        pop_task();
        
        enable_core_timer();
        // core_timer_handler();
    }

}

void el0_sync_entry(trap_frame_t* tpf){

    // uint64_t spsr_el1;
	// asm volatile("mrs %0, spsr_el1;" : "=r" (spsr_el1) :  : "memory");      
    
    // uint64_t elr_el1;
	// asm volatile("mrs %0, elr_el1;" : "=r" (elr_el1) :  : "memory");

    // uint64_t esr_el1;
	// asm volatile("mrs %0, esr_el1;" : "=r" (esr_el1) :  : "memory");

    // print_str("\nspsr_el1: 0x");
    // print_hex(spsr_el1);
    
    // print_str("\nelr_el1: 0x");
    // print_hex(elr_el1);

    // print_str("\nesr_el1: 0x");
    // print_hex(esr_el1);



    disable_interrupt();

    uint64_t syscall_no = tpf->x8;

    // print_str("\nsyscall_no: ");
    // print_dec(syscall_no);

    switch(syscall_no){
        case 0:
            int pid = sys_getpid();
            tpf->x0 = pid;
            break;
        case 1:
            enable_interrupt();
            uint32_t rlen = sys_uart_read(tpf->x0, tpf->x1);
            tpf->x0 = rlen;
            disable_interrupt();
            break;
        case 2:
            uint32_t wlen = sys_uart_write(tpf->x0, tpf->x1);
            tpf->x0 = wlen;
            break;
        case 3:
            int exec_state = sys_exec(tpf->x0, tpf->x1);
            tpf->elr_el1 = cur_thread->prog;
            tpf->sp_el0 = cur_thread->user_sp + USTACK_SIZE;
            tpf->x0 = exec_state;
            break;
        case 4:
            int child_pid = sys_fork(tpf);
            tpf->x0 = child_pid;
            break;
        case 5:
            sys_exit();
            break;
        case 6:
            int mbox_state = sys_mbox_call(tpf->x0, tpf->x1);
            tpf->x0 = mbox_state;
            break;
        case 7:
            sys_kill(tpf->x0);
            break;
        default:
            break;
    }



    enable_interrupt();

}