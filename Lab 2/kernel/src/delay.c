/* Wait N CPU cycles (ARM CPU only) */

void delay_cycles(unsigned int n)
{
    if (n) while (n--) { asm volatile("nop"); }
}
