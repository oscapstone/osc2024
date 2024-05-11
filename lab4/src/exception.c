#include "exception.h"
#include "mini_uart.h"
#include "peripherals/irq.h"
#include "timer.h"
#include "tasklist.h"

extern timer_t *timer_head;

void el1_interrupt_enable(){
    asm volatile ("msr daifclr, 0xf"); // umask all DAIF
}

void el1_interrupt_disable(){
    asm volatile ("msr daifset, 0xf"); // mask all DAIF
}

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
}

void irq_exception_handler_c(){
    // uart_send_string("IRQ Exception Occurs!\n");
    // el1_interrupt_disable();
    unsigned int irq = *IRQ_PENDING_1;
    unsigned int interrupt_source = *CORE0_INTERRUPT_SOURCE;

    if((irq & IRQ_PENDING_1_AUX_INT) && (interrupt_source & INTERRUPT_SOURCE_GPU)){
        // uart_send_string("UART interrupt\n");
        uart_irq_handler();
    } else if(interrupt_source & INTERRUPT_SOURCE_CNTPNSIRQ) {
        // uart_send_string("\nTimer interrupt\n");
        *CORE0_TIMER_IRQ_CTRL = 0;
        create_task(irq_timer_exception, 0);
        execute_tasks_preemptive();
        // put32(CORE0_TIMER_IRQ_CTRL, 2);
        // irq_timer_exception();
    }
    // execute_tasks();
    // el1_interrupt_enable();
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
            timer_head = timer_head -> next;
            if(timer_head){
                timer_head -> prev = 0;
            }
        } else {
            break;
        }

        if(timer_head){
            asm volatile("msr cntp_cval_el0, %0"::"r"(timer_head -> timeout));
            asm volatile("msr cntp_ctl_el0, %0"::"r"(1));
        } else {
            asm volatile("msr cntp_ctl_el0, %0"::"r"(0));
        }
    }
    

    // unsigned long long cntpct_el0 = 0;
    // asm volatile("mrs %0, cntpct_el0":"=r"(cntpct_el0));

    // unsigned long long cntfrq_el0 = 0;
    // asm volatile("mrs %0, cntfrq_el0":"=r"(cntfrq_el0));

    // unsigned long long sec = cntpct_el0 / cntfrq_el0;
    // uart_send_string("sec:");
    // uart_hex(sec);
    // uart_send_string("\n");

    // unsigned long long wait = cntfrq_el0 * 2;// wait 2 seconds
    // asm volatile ("msr cntp_tval_el0, %0"::"r"(wait));

}