#include "../include/mini_uart.h"
#include "../include/timer.h"
#include "../include/timer_utils.h"
#include "../include/exception.h"
#include "../include/peripherals/mini_uart.h"

void enable_interrupt() { asm volatile("msr DAIFClr, 0xf"); }
void disable_interrupt() { asm volatile("msr DAIFSet, 0xf"); }


static void show_exception_info()
{
	uint64_t spsr = read_sysreg(spsr_el1);
	uint64_t elr  = read_sysreg(elr_el1);
	uint64_t esr  = read_sysreg(esr_el1);
	uart_send_string("spsr_el1: 0x");
	uart_hex(spsr);
	uart_send_string("\r\n");
	uart_send_string("elr_el1: 0x");
	uart_hex(elr);
	uart_send_string("\r\n");
	uart_send_string("esr_el1: 0x");
	uart_hex(esr);
	uart_send_string("\r\n");
}

void exception_invalid_handler()
{
	disable_interrupt();
	uart_send_string("---In exception_invalid_handler---\r\n");
	show_exception_info();
	enable_interrupt();
}

void except_handler_c() 
{
	disable_interrupt();
	uart_send_string("---In exception_el0_sync_handler---\r\n");
	show_exception_info();
	enable_interrupt();
}

void timer_handler()
{
	// disable_interrupt();
	// uart_send_string("In timer handle\n");
	// uint64_t current_time = get_current_time();
	// uart_send_string("Current time: ");
	// uart_hex(current_time);
	// uart_send_string(" s  \r\n");
	// set_expired_time(2);
	// enable_interrupt();
}

void exception_el1_irq_handler()
{
	disable_interrupt();
	if (*CORE0_INTERRUPT_SOURCE & 0x2) {
		disable_timer_interrupt();
		// timer_handler();
		timer_interrupt_handler();
	}
	else if (*AUX_MU_IIR_REG & 0x4) {
		clr_rx_interrupts();
		uart_rx_handler();
	}
	else if (*AUX_MU_IIR_REG & 0x2) {
		clr_tx_interrupts();
		uart_tx_handler();
	}
	enable_interrupt();
	uart_send_string("> ");
}