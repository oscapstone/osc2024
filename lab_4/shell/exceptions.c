#include "include/uart.h"

int exception_entry(){
    uart_puts("exception entry!\n");

    unsigned long long spsr_el1, elr_el1, esr_el1;

    asm volatile(
    "mrs %0, spsr_el1;"
    "mrs %1, elr_el1;"
    "mrs %2, esr_el1;"
        : "=r" (spsr_el1), "=r" (elr_el1), "=r" (esr_el1)
    );
    uart_puts("spsr_el1: ");
    uart_hex(spsr_el1);
    uart_send('\n');
    uart_puts("elr_el1: ");
    uart_hex(elr_el1);
    uart_send('\n');
    uart_puts("esr_el1: ");
    uart_hex(esr_el1);
    uart_send('\n');

  return 0;
}