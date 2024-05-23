#include <stdint.h>

#include "exception.h"
#include "mini_uart.h"
#include "peripherals/irq.h"
#include "timer.h"
#include "tasklist.h"
#include "syscall.h"
#include "alloc.h"

extern timer_t *timer_head;

void el1_interrupt_enable(){
    asm volatile ("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable(){
    asm volatile ("msr daifset, 0xf"); // mask all DAIF
}

// void core_timer_enable() {
//     // asm volatile("msr cntp_ctl_el0, %0"::"r"(0));
//     asm volatile("msr cntp_ctl_el0, %0"::"r"(1));
//     unsigned long freq;
//     asm volatile("mrs %0, cntfrq_el0" : "=r" (freq));
//     freq = freq >> 5;
//     asm volatile("msr cntp_tval_el0, %0"::"r"(freq));
//     *CORE0_TIMER_IRQ_CTRL = 2;
//     // asm volatile(
//     //     "mrs x9, cntfrq_el0;"
//     //     "msr cntp_tval_el0, x9;"
//     //     "mov x9, 1;"
//     //     "msr cntp_ctl_el0, x9;"
//     // );
// }

void exception_handler_c() {
    uart_send_string("Exception Occurs!\n");

    //read spsr_el1
	unsigned long long spsr_el1 = 0;
	asm volatile("mrs %0, spsr_el1":"=r"(spsr_el1));
	uart_send_string("spsr_el1: ");
	uart_hex(spsr_el1);
	uart_send_string("\n");

	//read elr_el1
	unsigned long long elr_el1 = 0;
	asm volatile("mrs %0, elr_el1":"=r"(elr_el1));
	uart_send_string("elr_el1: ");
	uart_hex(elr_el1);
	uart_send_string("\n");
	
	//esr_el1
	unsigned long long esr_el1 = 0;
	asm volatile("mrs %0, esr_el1":"=r"(esr_el1));
    uart_send_string("esr_el1: ");
	uart_hex(esr_el1);
	uart_send_string("\n");

	//ec
	unsigned ec = (esr_el1 >> 26) & 0x3F; //0x3F = 0b111111(6)
	uart_send_string("ec: ");
	uart_hex(ec);
	uart_send_string("\n");
    reset(200);
}

void user_irq_exception_handler_c() {
    // uart_send_string("User IRQ Exception Occurs!\n");
    el1_interrupt_disable();
    unsigned int irq = *IRQ_PENDING_1;
    unsigned int interrupt_source = *CORE0_INTERRUPT_SOURCE;

    if((irq & IRQ_PENDING_1_AUX_INT) && (interrupt_source & INTERRUPT_SOURCE_GPU)){
        // uart_send_string("UART interrupt\n");
        uart_irq_handler();
    }
    if(interrupt_source & INTERRUPT_SOURCE_CNTPNSIRQ) {
        // uart_send_string("\nuser Timer interrupt");
        schedule();
        *CORE0_TIMER_IRQ_CTRL = 0;
        core_timer_disable();
        create_task(user_irq_timer_exception, 10);
        core_timer_enable();
        *CORE0_TIMER_IRQ_CTRL = 2;
        el1_interrupt_enable();
        execute_tasks_preemptive();        
    }
    asm volatile("msr cntp_ctl_el0,%0"::"r"(1));
    uint64_t freq;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    freq = freq >> 5;
    asm volatile("msr cntp_tval_el0, %0" : : "r"(freq));
}

void irq_exception_handler_c(){
    // uart_send_string("IRQ Exception Occurs!\n");
    el1_interrupt_disable();
    unsigned int irq = *IRQ_PENDING_1;
    unsigned int interrupt_source = *CORE0_INTERRUPT_SOURCE;

    if((irq & IRQ_PENDING_1_AUX_INT) && (interrupt_source & INTERRUPT_SOURCE_GPU) ){
        // uart_send_string("kernel UART interrupt\n");
        uart_irq_handler();
    }
    if(interrupt_source & INTERRUPT_SOURCE_CNTPNSIRQ) {
        // uart_send_string("\nkernel Timer interrupt");
        // free_list_info();
		// print_chunk_info();
        schedule();
        *CORE0_TIMER_IRQ_CTRL = 0;
        core_timer_disable();
        create_task(irq_timer_exception, 10);
        core_timer_enable();
        *CORE0_TIMER_IRQ_CTRL = 2;
        el1_interrupt_enable();
        execute_tasks_preemptive();
        // *CORE0_TIMER_IRQ_CTRL = 2;
        // put32(CORE0_TIMER_IRQ_CTRL, 2);
        // irq_timer_exception();
    }
    
    // execute_tasks();
}

void user_exception_handler_c(trapframe_t* tf) {
    int syscall_code = tf -> x[8];
    // uart_send_string("[INFO] Enter user except, syscall: ");
    // uart_hex(syscall_code);
    // uart_send_string("\n");
    el1_interrupt_enable();

    //     unsigned long far_el1, esr_el1, elr_el1, spsr_el1;
    
    // asm volatile("mrs %0, FAR_EL1" : "=r" (far_el1));
    // asm volatile("mrs %0, ESR_EL1" : "=r" (esr_el1));
    // asm volatile("mrs %0, ELR_EL1" : "=r" (elr_el1));
    // asm volatile("mrs %0, SPSR_EL1" : "=r" (spsr_el1));
    
    // uart_send_string("[EXCEPTION] FAR_EL1: ");
    // uart_hex(far_el1);
    // uart_send_string("\n");

    // uart_send_string("[EXCEPTION] ESR_EL1: ");
    // uart_hex(esr_el1);
    // uart_send_string("\n");

    // uart_send_string("[EXCEPTION] ELR_EL1: ");
    // uart_hex(elr_el1);
    // uart_send_string("\n");

    // uart_send_string("[EXCEPTION] SPSR_EL1: ");
    // uart_hex(spsr_el1);
    // uart_send_string("\n");
    // uart_send_string("[EXCEPTION] Trapframe:\n");
    // uart_hex(tf->x[0]);
    // uart_send_string("\n");
    // uart_hex(tf->sp_el0);
    // uart_send_string("\n");
    // uart_hex(tf->spsr_el1);
    // uart_send_string("\n");
    // uart_hex(tf->elr_el1);
    // uart_send_string("\n");

    switch (syscall_code) {
        case 0:
            tf -> x[0] = getpid();
            break;
        case 1:
            tf -> x[0] = uart_read(tf -> x[0], tf -> x[1]);
            break;
        case 2:
            tf -> x[0] = uart_write(tf -> x[0], tf -> x[1]);
            break;
        case 3:
            tf -> x[0] = exec((const char*)tf -> x[0], tf->x[1]);
            break;
        case 4:
            // uart_send_string("[INFO] system call: fork\n");
            el1_interrupt_disable();
            // *CORE0_TIMER_IRQ_CTRL = 0;
            // core_timer_disable();
            tf -> x[0] = fork(tf);
            // core_timer_enable();
            // *CORE0_TIMER_IRQ_CTRL = 2;
            el1_interrupt_enable();
            break;
        case 5:
            exit(0);
            break;
        case 6:
            el1_interrupt_enable();
            tf -> x[0] = mbox_call(
                (unsigned char)tf -> x[0], (unsigned int*)tf -> x[1]
            );
            el1_interrupt_disable();
            break;
        case 7:
            kill((int)tf -> x[0]);
            break;
    }
    return;
}

void irq_timer_exception(){
    // // enable timer iterrupt
    *CORE0_TIMER_IRQ_CTRL = 2;

    // disable timer interrupt before entering critical section
    // asm volatile("msr cntp_ctl_el0,%0"::"r"(0));

    while(timer_head){
        unsigned long long current_time;
        asm volatile("mrs %0, cntpct_el0":"=r"(current_time));
        if(timer_head -> timeout <= current_time) {
            timer_t *timer = timer_head;
            timer -> callback(timer -> data);
            kfree(timer);
            timer_head = timer_head -> next;
            if(timer_head){
                timer_head -> prev = 0;
            }
        } else {
            break;
        }

        if(timer_head){
            asm volatile("msr cntp_ctl_el0, %0"::"r"(1));
            asm volatile("msr cntp_cval_el0, %0"::"r"(timer_head -> timeout));
        } else {
            asm volatile("msr cntp_ctl_el0, %0" :: "r"(0));
        }
    }
}

void user_irq_timer_exception(){
    // // enable timer iterrupt
    // *CORE0_TIMER_IRQ_CTRL = 0;

    // disable timer interrupt before entering critical section
    while(timer_head){
        unsigned long long current_time;
        asm volatile("mrs %0, cntpct_el0":"=r"(current_time));
        if(timer_head -> timeout <= current_time) {
            timer_t *timer = timer_head;
            timer -> callback(timer -> data);
            kfree(timer);
            timer_head = timer_head -> next;
            if(timer_head){
                timer_head -> prev = 0;
            }
        } else {
            break;
        }

        if(timer_head){
            asm volatile("msr cntp_ctl_el0, %0"::"r"(1));
            asm volatile("msr cntp_cval_el0, %0"::"r"(timer_head -> timeout));
        } else {
            asm volatile("msr cntp_ctl_el0, %0" :: "r"(0));
        }
    }
}

void core_timer_disable() {
    asm volatile("msr cntp_ctl_el0, %0"::"r"(0));
}

void core_timer_enable(){
  uint64_t tmp;
  asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
  tmp |= 1;
  asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
  
  asm volatile(
    "mov x0, 1;"
    "msr cntp_ctl_el0, x0;" // enable
  );

  uint64_t freq;
  asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
  freq = freq >> 5;
  asm volatile("msr cntp_tval_el0, %0" : : "r"(freq));

  asm volatile(
    // "mrs x0, cntfrq_el0;"
    // "msr cntp_tval_el0, x0;" // set expired time
    "mov x0, 2;"
    "ldr x1, =0x40000040;"
    "str w0, [x1];" // unmask timer interrupt
  );
}

