#include "irq.h"
#include "timer.h"

void enable_interrupt()
{
    asm volatile("msr DAIFClr, 0xF;");
}

void disable_interrupt()
{
    asm volatile("msr DAIFSet, 0xF;");
}

void irq_entry()
{
    timer_irq_handler();
}