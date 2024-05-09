#include "interrupt.h"
#include "printf.h"

void enable_interrupt() {
    asm volatile("msr DAIFClr, 0xf");
    }
void disable_interrupt() { 
    asm volatile("msr DAIFSet, 0xf");
    }