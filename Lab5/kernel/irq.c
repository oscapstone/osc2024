#include "irq.h"

void irq_vector_init(void) {
    asm volatile (
        "adr x0, vectors;"
        "msr vbar_el1, x0;"
        "ret;"
    );
}

void enable_el1_interrupt(void) {
    asm volatile(
        "msr DAIFClr, 0xf;"
    );
}

void disable_el1_interrupt(void) {
    asm volatile(
        "msr DAIFSet, 0xf;"
    );
}