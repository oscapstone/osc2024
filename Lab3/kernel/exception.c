#include "exception.h"

void disable_interrupt(){
    asm volatile("msr DAIFSet, 0xf");
}

void enable_interrupt(){
    asm volatile("msr DAIFClr, 0xf");
}

void exception_entry(){
    disable_interrupt();
    
    uint64_t spsr_el1;
	asm volatile("mrs %0, SPSR_EL1;" : "=r" (spsr_el1) :  : "memory");      

    uint64_t elr_el1;
	asm volatile("mrs %0, ELR_EL1;" : "=r" (elr_el1) :  : "memory");

    uint64_t esr_el1;
	asm volatile("mrs %0, ESR_EL1;" : "=r" (esr_el1) :  : "memory");

    print_str("\nspsr_el1: 0x");
    print_hex(spsr_el1);
    
    print_str("\nelr_el1: 0x");
    print_hex(elr_el1);

    print_str("\nesr_el1: 0x");
    print_hex(esr_el1);

    enable_interrupt();
}