#include "irq.h"
#include "timer.h"

void enable_interrupt()
{
	asm volatile("msr DAIFClr, 0xf;");
}

void disable_interrupt()
{
	asm volatile("msr DAIFSet, 0xf;");
}

void irq_entry()
{
	timer_irq_handler();
}