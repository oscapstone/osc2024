#include "timer.h"
#include "uart.h"
#include "memory.h"

static struct event *timers = 0;

void infinite() {
    while (1);
}

uint64_t get_core_frequency() {
    uint64_t frequency;
    asm volatile("mrs %0, cntfrq_el0\r\n" :"=r"(frequency));
    return frequency;
}

uint64_t get_core_count() {
    uint64_t count;
    asm volatile("mrs %0, cntpct_el0\r\n" :"=r"(count));
    return count;
}

void core_timer_enable() {
    asm volatile( "mov    x0, 1\r\n\t" );
    asm volatile( "msr    cntp_ctl_el0, x0\r\n\t");     // enable
    asm volatile( "mov    x0, 2\r\n\t");                // set expired time
    asm volatile( "ldr    x1, =0x40000040\r\n\t");      // CORE0_TIMER_IRQ_CTRL
    asm volatile( "str    w0, [x1]\r\n\t");             // unmask timer interrupt

}

void core_timer_disable() {
    asm volatile( "mov    x0, 0\r\n\t" );
    asm volatile( "msr    cntp_ctl_el0, x0\r\n\t");     // disable
    asm volatile( "mov    x0, 2\r\n\t");                // set expired time
    asm volatile( "ldr    x1, =0x40000040\r\n\t");      // CORE0_TIMER_IRQ_CTRL
    asm volatile( "str    w0, [x1]\r\n\t");             // unmask timer interrupt

}

void add_core_timer(void (*func)(void*), void *args, uint32_t time) {
    struct event *timer = (struct event*) simple_malloc(sizeof(struct event));
    timer->args = args;
    timer->expired_time = get_core_count() + time;
    timer->callback = func;
    timer->next = 0;

    int update = 0;

    if (!timers) {
        timers = timer;
        update = 1;
    } else if (timers->expired_time > timer->expired_time) {
        //sorting timers and new timer is the bigger then directly put back
        timer->next = timers;
        timers = timer;
        update = 1;
    } else {
        //if newest not largest then sort and put
        struct event *current = timers;
        while (current->next && current->next->expired_time < timer->expired_time) {
            current = current->next;
        }

        timer->next = current->next;
        current->next = timer;
    }

    if (update) {
        set_core_timer(time);
    }
}

void set_core_timer(uint32_t time) {
    asm volatile("msr cntp_tval_el0, %0\r\n" :: "r"(time)); 
}

void remove_core_timer() {
    struct event *timer = timers;
    timer->callback(timer->args);
    timers = timers->next;

    if (!timers) {
        core_timer_disable();
    } else {
        uint32_t count = get_core_count();
        set_core_timer(timers->expired_time - count);
    }
}

void print_num(uint64_t value) {
    if (value == 0) {
        uart_puts("0"); return;
    }

    char nums[20]; unsigned int len = 0;

    while (value) {
        unsigned int x = value % 10;
        nums[len++] = '0' + x;
        value /= 10;
    }

    for (int i = len - 1; i >= 0; i--) {
        uart_send(nums[i]);
    }
}

void print_boot_time() {
    uart_puts("Core Timer Interrupt!\r\n");
    
    uint64_t frequency = get_core_frequency();
    uint64_t current   = get_core_count();

    uart_puts("Time after booting: ");
    print_num(current / frequency);
    uart_puts("(secs)\n");

    set_core_timer(2 * get_core_frequency());
}

void print_core_timer_message(void *args) {
    print_boot_time();
    uart_puts((char*) args);
    uart_puts("\n");
}

