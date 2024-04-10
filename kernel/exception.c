#include "exception.h"
#include "uart.h"

void init_exception_vectors(void)
{
    // set EL1 exception vector table
    asm("adr x0, exception_vector_table");
    asm("msr vbar_el1, x0");
}

void handle_exception(void)
{
    // print spsr_el1, elr_el1, esr_el1
    long spsr_el1 = 87;
    asm("mrs %0, spsr_el1" : "=r"((long) spsr_el1));

    uart_puts("spsr_el1: 0x");
    uart_hex(spsr_el1);
    uart_send('\n');

    long elr_el1 = 87;
    asm("mrs %0, elr_el1" : "=r"((long) elr_el1));

    uart_puts("elr_el1: 0x");
    uart_hex(elr_el1);
    uart_send('\n');

    long esr_el1 = 87;
    asm("mrs %0, esr_el1" : "=r"((long) esr_el1));

    uart_puts("esr_el1: 0x");
    uart_hex(esr_el1);
    uart_send('\n');

}
