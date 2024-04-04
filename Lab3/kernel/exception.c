#include "exception.h"

void disable_interrupt(){
    asm volatile("msr DAIFSet, 0xf");
}

void exception_entry(){
    disable_interrupt();
    
}