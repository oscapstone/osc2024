

#include "irq.h"
#include "timer.h"
#include "utils/printf.h"
#include "utils/utils.h"
#include "arm/arm.h"
#include "proc/task.h"
#include "mm/mm.h"

const U32 interval_1 = CLOCKHZ / 300;
U32 current_value_1 = 0;

TIMER_MANAGER timer_manager;

void core0_timer_enable();

void timer_init() {

    memzero(&timer_manager, sizeof(TIMER_MANAGER));

    // system timer
    //current_value_1 = REGS_TIMER->counter_low;
    //current_value_1 += interval_1;
    //REGS_TIMER->compare[1] = current_value_1;

    // enable access in EL0 for lab5 (armv8 pg. 2177)
    U64 timer_ctl_value = utils_read_sysreg(cntkctl_el1);
    timer_ctl_value |= 1;
    utils_write_sysreg(cntkctl_el1, timer_ctl_value);

}

void core0_timer_enable(U32 millisecond) {

    // core0 timer init
    utils_write_sysreg(cntp_ctl_el0, 1); // enable
    U32 freq = utils_read_sysreg(cntfrq_el0);
    utils_write_sysreg(cntp_tval_el0, freq * millisecond / 1000);
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

    U64 flags = irq_disable();
    if (timer_manager.tasks) {
        TIMER_TASK* current_task = timer_manager.tasks;
        void (*callback)() = current_task->callback;
        timer_manager.tasks = current_task->next;
        kfree(current_task);

        if (timer_manager.tasks) {
            core0_timer_enable(timer_manager.tasks->timeout);
        }
        irq_restore(flags);
        callback();
        return;
    }
    irq_restore(flags);

    // just use the core 0 timer to do multitasking
    // U64 freq = utils_read_sysreg(cntfrq_el0) / 1000;
    // utils_write_sysreg(cntp_tval_el0, freq * 30LL);
    // U64 freq = utils_read_sysreg(cntfrq_el0);
    // utils_write_sysreg(cntp_tval_el0, freq);
    // printf("[CORE TIMER TEST]\n");
    
    // when handling interrupts default state is disable interrupt so we need to enable it
    // enable_interrupt();
    // task_schedule();
    //disable_interrupt();        // 老實說根本不會走到這裡八?
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

void timer_add(void (*callback)(), U32 millisecond) {

    U64 flags = irq_disable();
    TIMER_TASK* timer_task = kmalloc(sizeof(TIMER_TASK));
    timer_task->callback = callback;
    timer_task->next = NULL;
    timer_task->timeout = millisecond;

    if (!timer_manager.tasks) {
        timer_manager.tasks = timer_task;

        // setup the next interrupt
        core0_timer_enable(timer_manager.tasks->timeout);
    } else {

        TIMER_TASK* current_task = timer_manager.tasks;
        TIMER_TASK* next_task = current_task->next;

        if (current_task->timeout > timer_task->timeout) {
            timer_manager.tasks = timer_task;
            timer_task->next = current_task;
            current_task->timeout -= timer_task->timeout;
            
            // setup the next interrupt
            core0_timer_enable(timer_manager.tasks->timeout);
            irq_restore(flags);
            return;
        }

        while (next_task) {
            if (current_task->timeout < timer_task->timeout && next_task->timeout >= timer_task->timeout) {
                current_task->next = timer_task;
                timer_task->next = next_task;
                timer_task->timeout -= current_task->timeout;

                irq_restore(flags);
                return;
            }

            // continue subtract because current_task is before
            timer_task->timeout -= current_task->timeout;

            current_task = next_task;
            next_task = next_task->next;
        }

        timer_task->timeout -= current_task->timeout;
        current_task->next = timer_task;
    }
    irq_restore(flags);
}