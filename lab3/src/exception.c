#include "exception.h"
#include "mini_uart.h"
#include "utils.h"
#include "peripherals/irq.h"

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

    unsigned int irq = get32(IRQ_PENDING_1);
    unsigned int interrupt_source = get32(CORE0_INTERRUPT_SOURCE);

    if((irq & IRQ_PENDING_1_AUX_INT) && (interrupt_source & INTERRUPT_SOURCE_GPU)){
        // uart_send_string("UART interrupt\n");
        uart_irq_handler();
    } else if(interrupt_source & INTERRUPT_SOURCE_CNTPNSIRQ) {
        uart_send_string("Timer interrupt\n");
        irq_timer_exception();
    }
}

void irq_timer_exception(){
    unsigned long long cntpct_el0 = 0;
    asm volatile("mrs %0, cntpct_el0":"=r"(cntpct_el0));

    unsigned long long cntfrq_el0 = 0;
    asm volatile("mrs %0, cntfrq_el0":"=r"(cntfrq_el0));

    unsigned long long sec = cntpct_el0 / cntfrq_el0;
    uart_send_string("sec:");
    uart_hex(sec);
    uart_send_string("\n");

    unsigned long long wait = cntfrq_el0 * 2;// wait 2 seconds
    asm volatile ("msr cntp_tval_el0, %0"::"r"(wait));

}