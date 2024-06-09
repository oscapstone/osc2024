#include "timer.h"
#include "alloc.h"
#include "string.h"
#include "type.h"
#include "io.h"

extern void timer_tick();

void timer_init()
{
    core_timer_enable();
    set_timer(1);
}

void irq_timer_handler()
{
    set_timer_asm(get_cpu_freq()>>5);
    timer_tick();
}

void set_timer(uint32_t sec)
{
    uint64_t cycles = get_cpu_freq() * sec;
    set_timer_asm(cycles);
}
