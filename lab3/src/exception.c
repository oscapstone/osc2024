#include "../include/mini_uart.h"
#include "../include/timer.h"
#include "../include/timer_utils.h"
#include "../include/exception.h"

void enable_interrupt() { asm volatile("msr DAIFClr, 0xf"); }
void disable_interrupt() { asm volatile("msr DAIFSet, 0xf"); }

void except_handler_c() 
{
	uart_send_string("In default handle\n");

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
	uart_hex(esr_el1);
	uart_send_string("\n");

	//ec
	unsigned ec = (esr_el1 >> 26) & 0x3F; //0x3F = 0b111111(6)
	uart_send_string("ec: ");
	uart_hex(ec);
	uart_send_string("\n");
}

void low_irq_handler_c()
{
	disable_interrupt();
	uart_send_string("In timer handle\n");
	uint64_t current_time = get_current_time();
	uart_send_string("Current time: ");
	uart_hex(current_time);
	uart_send_string(" s  \r\n");
	set_expired_time(2);
	enable_interrupt();
}

	