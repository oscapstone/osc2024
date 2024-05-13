

#include "irq.h"
#include "timer.h"
#include "utils/printf.h"
#include "utils/utils.h"
#include "arm/arm.h"
#include "proc/task.h"

const U32 interval_1 = CLOCKHZ / 300;
U32 current_value_1 = 0;

void core0_timer_enable();

void timer_init() {

    // system timer
    current_value_1 = REGS_TIMER->counter_low;
    current_value_1 += interval_1;
    REGS_TIMER->compare[1] = current_value_1;

    core0_timer_enable();
}

void core0_timer_enable() {

    // core0 timer init
    utils_write_sysreg(cntp_ctl_el0, 1); // enable

    U64 freq = utils_read_sysreg(cntfrq_el0) / 1000;
    // for test
    utils_write_sysreg(cntp_tval_el0, freq * 30LL);
    
    
    
    
    REGS_ARM_CORE->timer_control[0] = 2; // enable irq

}

void handle_timer_1() {
    
    // add next 1 second
    current_value_1 += interval_1;
    REGS_TIMER->compare[1] = current_value_1;

    // reset the timer 1 match
    REGS_TIMER->control_status |= SYS_TIMER_1;
}


void timer_sys_timer_3_handler() {
    // TODO
    //REGS_TIMER->compare[3];

    // reset the timer 3 match
    REGS_TIMER->control_status |= SYS_TIMER_3;
}

void timer_core_timer_0_handler() {
    // just use the core 0 timer to do multitasking
    U64 freq = utils_read_sysreg(cntfrq_el0) / 1000;
    utils_write_sysreg(cntp_tval_el0, freq * 30LL);
    
    // when handling interrupts default state is disable interrupt so we need to enable it
    enable_interrupt();
    task_schedule();
    disable_interrupt();        // 老實說根本不會走到這裡八?
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