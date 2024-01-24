#include "uart.h"
#include "exception.h"


/**
 * common exception handler
 */
void exc_handler(unsigned long type, unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
{
    // print out interruption type
    switch(type) {
        case 0: uart_puts("Synchronous"); break;
        case 1: uart_puts("IRQ"); break;
        case 2: uart_puts("FIQ"); break;
        case 3: uart_puts("SError"); break;
    }
    uart_puts(": ");
    // decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
    switch(esr >> 26) {
        case 0b000000: uart_puts("Unknown"); break;
        case 0b000001: uart_puts("Trapped WFI/WFE"); break;
        case 0b001110: uart_puts("Illegal execution"); break;
        case 0b010101: uart_puts("System call"); break;
        case 0b100000: uart_puts("Instruction abort, lower EL"); break;
        case 0b100001: uart_puts("Instruction abort, same EL"); break;
        case 0b100010: uart_puts("Instruction alignment fault"); break;
        case 0b100100: uart_puts("Data abort, lower EL"); break;
        case 0b100101: uart_puts("Data abort, same EL"); break;
        case 0b100110: uart_puts("Stack alignment fault"); break;
        case 0b101100: uart_puts("Floating point"); break;
        default: uart_puts("Unknown"); break;
    }
    switch(esr&0x3) {
        case 0: uart_puts(" at level 0"); break;
        case 1: uart_puts(" at level 1"); break;
        case 2: uart_puts(" at level 2"); break;
        case 3: uart_puts(" at level 3"); break;
    }
    // decode data abort cause
    if(esr >> 26 == 0b100100 || esr >> 26 == 0b100101) {
        uart_puts(", ");
        switch((esr >> 2) & 0x3) {
            case 0: uart_puts("Address size fault"); break;
            case 1: uart_puts("Translation fault"); break;
            case 2: uart_puts("Access flag fault"); break;
            case 3: uart_puts("Permission fault"); break;
        }
        switch(esr & 0x3) {
            case 0: uart_puts(" at level 0"); break;
            case 1: uart_puts(" at level 1"); break;
            case 2: uart_puts(" at level 2"); break;
            case 3: uart_puts(" at level 3"); break;
        }
    }
    // dump registers
    uart_puts(":\n  ESR_EL1 ");
    uart_hex(esr>>32);
    uart_hex(esr);
    uart_puts(" ELR_EL1 ");
    uart_hex(elr>>32);
    uart_hex(elr);
    uart_puts("\n SPSR_EL1 ");
    uart_hex(spsr>>32);
    uart_hex(spsr);
    uart_puts(" FAR_EL1 ");
    uart_hex(far>>32);
    uart_hex(far);
    uart_puts("\n");

    // no return from exception for now
    while(1);
}

void irq_router(unsigned long esr, unsigned long elr, unsigned long spsr, unsigned long far)
{
    // uart_puts("IRQ handler\n");
    int irq_core0 = *CORE0_IRQ_SOURCE;
    int irq_pend1 = *IRQ_PEND1;
    if (irq_core0 & 0x2) {
        uart_puts("Timer interrupt: ");
        core_timer_handler();
    } else if (irq_pend1 & (1 << 29)) {
        uart_interrupt_handler();
    }
}

uart_interrupt_handler()
{
    if (*AUX_MU_IIR & 0x2) { // Transmit holds register empty
        uart_puts("Transmit holds register empty, should not be here right now\n"); // cause we only enable receive interrupt
    } else if (*AUX_MU_IIR & 0x4) { // Receiver holds valid bytes
        if (*AUX_MU_LSR & 0x1) { // Receiver FIFO holds valid bytes
            char r = (char) (*AUX_MU_IO); // If we take char from AUX_MU_IO, the interrupt will be cleared.
            r = (r == '\r') ? '\n' : r;

            // ouput the char to screen (uart_send without pooling)
            if (*AUX_MU_LSR & 0x20)
                *AUX_MU_IO = r;
        } else {
            uart_puts("Something unexpected\n");
        }
    } else 
        uart_puts("Something unexpected\n");
}

void core_timer_handler()
{
    int seconds;
    asm volatile(
        "mrs x0, cntpct_el0     \n\t"
        "mrs x1, cntfrq_el0     \n\t"
        "udiv %0, x0, x1        \n\t": "=r" (seconds));
    uart_puts("seconds: ");
    uart_hex(seconds);
    uart_send('\n');
    asm volatile(
        "mrs x0, cntfrq_el0     \n\t"
        "mov x1, 2              \n\t"
        "mul x0, x0, x1         \n\t"
        "msr cntp_tval_el0, x0  \n\t");
}