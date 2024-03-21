#include "irq.h"
#include "timer.h"
#include "uart.h"

void enable_interrupt()
{
    // Clear the D, A, I, F bits in the DAIF register
    // to enable interrupts in EL1
    asm volatile("msr DAIFClr, 0xF;");
}

void disable_interrupt()
{
    // Set 1 to the D, A, I, F bits in the DAIF register
    // to disable interrupts in EL1
    asm volatile("msr DAIFSet, 0xF;");
}

void irq_entry()
{
    // Enter the critical section ->
    // temporarily disable interrupts
    disable_interrupt();

    // if (*IRQ_PENDING_1 & (1 << 29)) {
    //     uart_irq_handler();
    // } else {
    //     timer_irq_handler();
    // }

    enable_interrupt();
}