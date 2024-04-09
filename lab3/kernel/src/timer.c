#include "timer.h"
#include "uart.h"
void core_timer_enable() {
    asm volatile( "mov    x0, 1" );
    asm volatile( "msr    cntp_ctl_el0, x0");     // enable
    asm volatile( "mov    x0, 2");                // set expired time
    asm volatile( "ldr    x1, =0x40000040");      // CORE0_TIMER_IRQ_CTRL
    asm volatile( "str    w0, [x1]");             // unmask timer interrupt

}

uint64_t get_core_frequency() {
    uint64_t frequency;
    asm volatile("mrs %0, cntfrq_el0" :"=r"(frequency));
    return frequency;
}

void set_core_timer(uint32_t time) {
    asm volatile("msr cntp_tval_el0, %0" :"=r"(time)); 
    //set timeout => wait 2*freq times
}

uint64_t get_core_count() {
    uint64_t count;
    asm volatile("mrs %0, cntpct_el0" :"=r"(count));
    return count;
}

void infinite() {
    while (1);
}

void print_boot_time() {
    uart_puts("Core Timer Interrupt!\r\n");
    

    uint64_t frequency = get_core_frequency();
    uint64_t current   = get_core_count();

    uart_puts("Time after booting: ");
    uart_hex(current / frequency);
    uart_puts("(s)");

    set_core_timer(2 * get_core_frequency());
}
