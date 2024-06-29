#include <stdint.h>

#include "uart.h"

void print_registers(uint64_t elr, uint64_t esr, uint64_t spsr) {
  // Print spsr_el1
  uart_puts("spsr_el1: ");
  uart_hex(spsr);
  uart_putc(NEWLINE);

  // Print elr_el1
  uart_puts(" elr_el1: ");
  uart_hex(elr);
  uart_putc(NEWLINE);

  // Print esr_el1
  uart_puts(" esr_el1: ");
  uart_hex(esr);
  uart_putc(NEWLINE);
  uart_putc(NEWLINE);
}

void exception_entry(uint64_t elr, uint64_t esr, uint64_t spsr) {
  uart_puts("Exception Handler");
  uart_putc(NEWLINE);
  print_registers(elr, esr, spsr);
}

void invalid_entry(uint64_t elr, uint64_t esr, uint64_t spsr) {
  uart_puts("[ERROR] The exception handler is not implemented");
  uart_putc(NEWLINE);
  print_registers(elr, esr, spsr);
  while (1);
}