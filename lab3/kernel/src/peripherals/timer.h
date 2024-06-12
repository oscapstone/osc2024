#pragma once

#include "base.h"
#include "bcm2837base.h"

// one second
#define CLOCKHZ 1000000

// bcm2837 pg. 172
struct timer_regs {
    REG32 control_status;
    REG32 counter_low;
    REG32 counter_high;
    REG32 compare[4];
};

#define REGS_TIMER ((struct timer_regs*)(PBASE + 0x00003000))

void timer_init();
void handle_timer_1();
U64 timer_get_ticks();
void timer_sleep(U32 ms);