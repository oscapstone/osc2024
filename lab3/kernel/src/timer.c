#include "timer.h"
#include "mini_uart.h"


#define XSTR(s) STR(s)
#define STR(s) #s

void core_timer_enable() {
    __asm__ __volatile__ (
        "mov x0, 1\n\t"
        "msr cntp_ctl_el0, x0\n\t"          // enable
        "mrs x0, cntfrq_el0\n\t"

        "mov x0, 2\n\t"
        "ldr x1, ="XSTR(CORE0_TIMER_IRQ_CTRL)"\n\t"
        "str w0, [x1]\n\t"                  // unmask timer interrupt
    );
}

void set_time_out(unsigned long long seconds) {
    unsigned long long freq, ticks;
    
    __asm__ __volatile__ (
        "mrs %0, cntfrq_el0" : "=r"(freq)
    );

    ticks = freq * seconds; 
    __asm__ __volatile__ (
        "msr cntp_tval_el0, %0" :: "r"(ticks)
    );
}

void timer_interrupt_handler() {
    // // 這裡插入你希望在時間到時執行的代碼
    // uart_puts("Timer interrupt triggered!\r\n");

    // // 重設計時器（例如每2秒）
    // set_time_out(2);
}
