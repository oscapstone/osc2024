#include "exception.h"
#include "uart.h"

void handle_exception(void)
{
    // print spsr_el1, elr_el1, esr_el1
    long spsr = 87;
    asm("mrs %0, spsr_el1" : "=r"(spsr));

    uart_puts("spsr_el1: ");
    uart_putlong(spsr);
    uart_puts("\n");
}
