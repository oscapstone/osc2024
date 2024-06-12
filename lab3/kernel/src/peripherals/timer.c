
#include "irq.h"
#include "timer.h"
#include "utils/printf.h"

const U32 interval_1 = CLOCKHZ;
U32 current_value_1 = 0;

void timer_init() {

    current_value_1 = REGS_TIMER->counter_low;
    current_value_1 += interval_1;
    REGS_TIMER->compare[1] = current_value_1;

}

void handle_timer_1() {
    current_value_1 += interval_1;
    REGS_TIMER->compare[1] = current_value_1;
    REGS_TIMER->control_status |= SYS_TIMER_1;
    //printf("Timer 1 received\n");
}

U64 timer_get_ticks() {
    U32 high = REGS_TIMER->counter_high;
    U32 low = REGS_TIMER->counter_low;

    // 可能會不一樣
    if (high != REGS_TIMER->counter_high) {
        high = REGS_TIMER->counter_high;
        low = REGS_TIMER->counter_low;
    }

    return ((U64) high << 32) | low;
}

// sleep in milliseconds
void timer_sleep(U32 ms) {

    U64 start = timer_get_ticks();

    while (timer_get_ticks() < start + (ms * 1000)) {
        asm volatile("nop");
    }
}