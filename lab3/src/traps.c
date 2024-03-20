#include "uart.h"

void exception_entry()
{
    uart_puts("Exception:\n");

    // Print spsr_el1
    unsigned long long spsr_el1 = 0;
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr_el1));
    uart_puts("spsr_el1: ");
    uart_hex(spsr_el1);
    uart_putc('\n');

    // Print elr_el1
    unsigned long long elr_el1 = 0;
    asm volatile("mrs %0, elr_el1" : "=r"(elr_el1));
    uart_puts(" elr_el1: ");
    uart_hex(elr_el1);
    uart_putc('\n');

    // Print esr_el1
    unsigned long long esr_el1 = 0;
    asm volatile("mrs %0, esr_el1" : "=r"(esr_el1));
    uart_puts(" esr_el1: ");
    uart_hex(esr_el1);
    uart_putc('\n');
}

void invalid_entry()
{
    uart_puts("Exception caught but its handler is not implemented.\n");
}