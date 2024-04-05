static unsigned int seconds = 0;

unsigned int get_seconds(void)
{
    return seconds;
}

void set_seconds(unsigned int s)
{
    seconds = s;
}

void set_core_timer_timeout(void)
{
    unsigned long freq = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    asm volatile("msr cntp_tval_el0, %0" ::"r"(seconds * freq));
}
