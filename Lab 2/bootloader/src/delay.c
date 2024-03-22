/* Wait N CPU cycles (ARM CPU only) */
#include "delay.h"


void 
delay_cycles(uint32_t n)
{
    if (n) while (n--) { asm volatile("nop"); }
}
