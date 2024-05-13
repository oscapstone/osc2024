#pragma once

#include "base.h"
#include "bcm2837base.h"

// one second for system timer
#define CLOCKHZ 1000000

// bcm2837 pg. 172
// this is system timer
struct timer_regs {
    REG32 control_status;
    REG32 counter_low;
    REG32 counter_high;
    REG32 compare[4];
};

#define REGS_TIMER ((struct timer_regs*)(PBASE + 0x00003000))

void timer_init();
void handle_timer_1();
void timer_sys_timer_3_handler();
void timer_core_timer_0_handler();
U64 timer_get_ticks();
void timer_sleep(U32 ms);