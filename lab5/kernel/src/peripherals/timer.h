#pragma once

#include "base.h"
#include "bcm2837base.h"

//
//
typedef struct _TIMER_TASK {
    U32 timeout;        // the timeout compare to the previous TASK in millisecond
    void (*callback)();
    struct _TIMER_TASK* next;
}TIMER_TASK;

typedef struct _TIMER_MANAGER
{
    TIMER_TASK* tasks;
}TIMER_MANAGER;

void timer_add(void (*callback)(), U32 millisecond);

//
// user timer

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