#include <stdint.h>

#include "uart.h"

void print_registers(uint64_t elr, uint64_t esr, uint64_t spsr)
{
    // Print spsr_el1
    uart_puts("spsr_el1: ");
    uart_hex(spsr);
    uart_putc('\n');

    // Print elr_el1
    uart_puts(" elr_el1: ");
    uart_hex(elr);
    uart_putc('\n');

    // Print esr_el1
    uart_puts(" esr_el1: ");
    uart_hex(esr);
    uart_puts("\n\n");
}

void exception_entry(uint64_t elr, uint64_t esr, uint64_t spsr)
{
    uart_puts("Exception Handler\n");
    print_registers(elr, esr, spsr);
}

void invalid_entry(uint64_t elr, uint64_t esr, uint64_t spsr)
{
    uart_puts("Exception caught but its handler is not implemented.\n");
    print_registers(elr, esr, spsr);
}