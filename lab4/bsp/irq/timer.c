#include <kernel/bsp_port/irq.h>
#include <kernel/io.h>

void core_timer_enable() {
#ifdef DEBUG
    print_string("\n[core_timer_enable] ");
#endif
    asm volatile(
        "mov x0, 1;"
        "msr cntp_ctl_el0, x0;"  // Enable
        "mrs x0, cntfrq_el0;"
        "msr cntp_tval_el0, x0;"  // Set expired time
        "mov x0, 2;"
        "ldr x1, =0x40000040;"
        "str w0, [x1];");  // Unmask timer interrupt
}

void core_timer_disable() {
#ifdef DEBUG
    print_string("\n[core_timer_disable] ");
#endif
    asm volatile(
        "mov x0, 0;"
        "msr cntp_ctl_el0, x0 // disable;"
        "ldr x1, =0x40000040;"
        "str w0, [x1];"  // Mask timer interrupt
    );
}

uint64_t timer_get_uptime() {
    uint64_t cntpct_el0 = 0;
    uint64_t cntfrq_el0 = 0;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));
    return cntpct_el0 / cntfrq_el0;
}
