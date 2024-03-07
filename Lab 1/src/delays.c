
/**
 * Wait N CPU cycles (ARM CPU only)
 */
void wait_cycles(unsigned int n)
{
    if (n) while (n--) { asm volatile("nop"); }
}

