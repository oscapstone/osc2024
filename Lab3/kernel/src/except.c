#include "uart.h"
#include "shell.h"
#include "time_c.h"
#include "except_c.h"
#include "interrupt.h"
#include "tasklist.h"
#include "printf.h"
#include "utils_h.h"
#include "utils.h"

#include <stdint.h>

#define CNTPSIRQ_BIT_POSITION 0x02
#define AUXINIT_BIT_POSTION 1<<29

void core_timer_exception_entry() {

	unsigned long long cntpct_el0 = 0;//The register count secs with frequency
	asm volatile("mrs %0,cntpct_el0":"=r"(cntpct_el0));
	unsigned long long cntfrq_el0 = 0;//The base frequency
	asm volatile("mrs %0,cntfrq_el0":"=r"(cntfrq_el0));

	unsigned long long sec = cntpct_el0 / cntfrq_el0;

	uart_display_string("\r");
	uart_display_string(timer_head->msg);
	printf(" <- show in %d sec (after booting up)",sec);
	uart_display_string("\n");

		
	// timer_head->msg = "";
	
	if (timer_head->next){
		timer_head = timer_head->next;
		task_head->prev = NULL;
		asm volatile ("msr cntp_cval_el0, %0"::"r"(timer_head->executeTime));//set new timer
		asm volatile ("bl core_timer_irq_enable");
	}
	else{
		timer_head = NULL;
		asm volatile ("bl core_timer_irq_disable");
	}
	
}
uint32_t count = 0;
uint64_t elr;
uint64_t spsr;
uint64_t nest_elr;
uint64_t nest_spsr;

void irq_exception_entry(){
	disable_interrupt();
	// printf("IN IRQ\n");
	count++;
	if(count == 1){
		asm volatile("mrs %0,elr_el1":"=r"(elr));
		asm volatile("mrs %0,spsr_el1":"=r"(spsr));
	}
	else if(count == 2){
		asm volatile("mrs %0,elr_el1":"=r"(nest_elr));
		asm volatile("mrs %0,spsr_el1":"=r"(nest_spsr));
	}
	
	uint32_t irq_pending1 = *IRQ_PENDING_1;
	uint32_t core0_interrupt_source = *CORE0_INTERRUPT_SOURCE;
	uint32_t uart = irq_pending1 & AUXINIT_BIT_POSTION;
	uint32_t core = core0_interrupt_source & 0x2;

	if(uart  || core){
		if(uart) {
			uart_disable_interrupt();
			create_task(uart_handler, 2);
			// uart_handler();
		}
		if(core){
			asm volatile("bl core_timer_irq_disable");
			create_task(core_timer_exception_entry,1);
			// core_timer_exception_entry();			
		}
	}
	else{
		asm volatile("bl core_timer_irq_disable");
		uart_disable_interrupt();
	}

	enable_interrupt();
	if(task_head && count == 1){
		execute_tasks(elr,spsr);
	}
	// printf("OUT IRQ\n");
	count--;
}

void exception_entry() {
	uart_display_string("\rIn Exception handler\n");

	//read spsr_el1
	unsigned long long spsr_el1 = 0;
	asm volatile("mrs %0, spsr_el1":"=r"(spsr_el1));
	uart_display_string("spsr_el1: ");
	uart_binary_to_hex(spsr_el1);
	uart_display_string("\n");

	//read elr_el1
	unsigned long long elr_el1 = 0;
	asm volatile("mrs %0, elr_el1":"=r"(elr_el1));
	uart_display_string("elr_el1: ");
	uart_binary_to_hex(elr_el1);
	uart_display_string("\n");
	
	//esr_el1
	unsigned long long esr_el1 = 0;
	asm volatile("mrs %0, esr_el1":"=r"(esr_el1));
	uart_display_string("esr_el1: ");
	uart_binary_to_hex(esr_el1);
	uart_display_string("\n");

	//ec
	unsigned ec = (esr_el1 >> 26) & 0x3F; 	// 0x3F == 0b111111(6)
	uart_display_string("ec: ");
	uart_binary_to_hex(ec);
	uart_display_string("\n");				// SVC instruction execution in AArch64 state.
}

void test_entry(){
	uart_display_string("I'm in the test entry\n");
}






