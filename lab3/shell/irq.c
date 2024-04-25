#include"header/irq.h"
#include"header/uart.h"
#include"header/shell.h"
#include "header/timer.h"
#include "header/uart.h"
#include "header/task.h"
#define CORE0_TIMER_IRQ_CTRL_ ((volatile unsigned int *)(0x40000040))
#define AUX_MU_IIR ((volatile unsigned int *)(0x3F215048))

void enable_interrupt() {
    asm volatile("msr DAIFClr, 0xf");
	//DAIFClr: This is the name of the register being targeted for modification. In ARM architectures, 
	//DAIF stands for "Disable Asynchronous Interrupts Flag". 
	//The Clr suffix likely indicates that the instruction is clearing specific bits in this register.
	//0xf: This is the value being written to the DAIF register. In binary, 0xf is 1111, 
	//meaning all four exception mask bits (Debug, SError, IRQ, and FIQ) are being cleared. 
	//By clearing these bits, interrupts and exceptions of all types are allowed to be taken.
}

void disable_interrupt() {
    asm volatile("msr DAIFSet, 0xf");
	//set bits 0xf
}

void except_handler_c(unsigned int x0) {
	uart_send_str("In Exception handle\n");
	// Set all bits in the DAIF (Disable Asynchronous Interrupts Flags) register to 1, effectively masking all interrupts and exceptions
	asm volatile("msr DAIFSet, 0xf");

	// Declare and initialize a variable to store the value of SPSR_EL1 (Saved Program Status Register for Exception Level 1)
	unsigned long long spsr_el1 = 0;

	// Read the value of SPSR_EL1 into the variable spsr_el1
	asm volatile("mrs %0, spsr_el1":"=r"(spsr_el1));

	// Transmit the value of SPSR_EL1 over UART
	uart_send_str("spsr_el1: ");
	uart_binary_to_hex(spsr_el1);
	uart_send_str("\r\n");

	// Declare and initialize a variable to store the value of ELR_EL1 (Exception Link Register for Exception Level 1)
	unsigned long long elr_el1 = 0;

	// Read the value of ELR_EL1 into the variable elr_el1
	asm volatile("mrs %0, elr_el1":"=r"(elr_el1));

	// Transmit the value of ELR_EL1 over UART
	uart_send_str("elr_el1: ");
	uart_binary_to_hex(elr_el1);
	uart_send_str("\r\n");

	// Declare and initialize a variable to store the value of ESR_EL1 (Exception Syndrome Register for Exception Level 1)
	unsigned long long esr_el1 = 0;

	// Read the value of ESR_EL1 into the variable esr_el1
	asm volatile("mrs %0, esr_el1":"=r"(esr_el1));

	// Transmit the value of ESR_EL1 over UART
	uart_binary_to_hex(esr_el1);
	uart_send_str("\r\n");

	// Extract and transmit the value of EC (Exception Class) from ESR_EL1 over UART
	unsigned ec = (esr_el1 >> 26) & 0x3F; // Extract the EC field from bits 32 to 26 in ESR_EL1
	uart_send_str("ec: ");
	uart_binary_to_hex(ec);
	uart_send_str("\n");

	// Clear all bits in the DAIF register, allowing interrupts and exceptions to be taken again
	asm volatile("msr DAIFClr, 0xf");

	while (1) {
	
	}
}

void irq_except_handler_c() {
	// part 2
	// asm volatile("msr DAIFSet, 0xf");
	// uart_send_str("In timer interruption\n");
	// unsigned long long cntpct_el0 = 0;//The register count secs with frequency
	// asm volatile("mrs %0,cntpct_el0":"=r"(cntpct_el0));
	// unsigned long long cntfrq_el0 = 0;//The base frequency
	// asm volatile("mrs %0,cntfrq_el0":"=r"(cntfrq_el0));
	// unsigned long long sec = cntpct_el0 / cntfrq_el0;
	// uart_send_str("sec:");//except_handler_c
	// uart_binary_to_hex(sec);
	// uart_send_str("\n");
	// unsigned long long wait = cntfrq_el0 * 2;// wait 2 seconds
	// asm volatile ("msr cntp_tval_el0, %0"::"r"(wait));//set new timer
	// asm volatile("msr DAIFClr, 0xf");

	// part3
	// from aux && from GPU0 -> uart exception
	// if(*IRQ_PENDING_1 & (1<<29) &&*CORE0_INTERRUPT_SOURCE & (1 << 8)) 
    // {
	// 	if (*AUX_MU_IIR & 0x4) {
    //         uart_rx_interrupt_disable();
    //         uart_rx_handler();
    //         // uart_puts("pop task1\n");
    //     }
    //     if (*AUX_MU_IIR & 0x2) {
    //         uart_tx_interrupt_disable();
    //         // pop_task();
    //         uart_tx_handler();
    //     }
	// }
	// else if (*CORE0_INTERRUPT_SOURCE & (1 << 1)) {
    //     core_timer_interrupt_disable();
    //     *CORE0_TIMER_IRQ_CTRL_ = 0;
    //     core_timer_handler();
    //     core_timer_interrupt_enable();
    // }

	//final
	// see p13
	if(*IRQ_PENDING_1 & (1<<29) && *CORE0_INTERRUPT_SOURCE & (1 << 8)) 
    {
		if (*AUX_MU_IIR & 0x4) {
            uart_rx_interrupt_disable();
			add_task(uart_rx_handler, 2);
            run_task();
        }
        if (*AUX_MU_IIR & 0x2) {
            uart_tx_interrupt_disable();
			add_task(uart_tx_handler, 1);
            run_task();
        }
	}
	else if (*CORE0_INTERRUPT_SOURCE & (1 << 1)) {
        core_timer_interrupt_disable();
		add_task(core_timer_handler, 0);
        run_task();
        core_timer_interrupt_enable();
    }
}
