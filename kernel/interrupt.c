#include "interrupt.h"
#include "uart.h"

void init_core_timer(void)
{

}

void handle_interrupt(void)
{
    uart_puts("interrupted!\n");

    // TODO: seconds after booting
    long count = 44;
    asm("mrs %0, cntpct_el0" : "=r"((long) count));

    uart_puts("count: ");
    uart_putlong(count);
    uart_send('\n');

    long freq = 45; //62500000 (62.5 MHz)
    asm("mrs %0, cntfrq_el0" : "=r"((long) freq));
    uart_putlong(freq);
    uart_send('\n');

    long cval = 45; //62500000 (62.5 MHz)
    asm("mrs %0, cntp_cval_el0" : "=r"((long) cval));
    uart_puts("cval: ");
    uart_putlong(cval);
    uart_send('\n');

    long tval = 45; //62500000 (62.5 MHz)
    asm("mrs %0, cntp_tval_el0" : "=r"((long) tval));
    uart_puts("tval: ");
    uart_putlong(tval);
    uart_send('\n');

    uart_puts("second: ");
    uart_putints(count/freq);
    uart_send('\n');

    uart_send('\n');
    

    // TODO: set next timeout (what?)
    // reset timer (cntp_tval_el0 = ???)
    asm("mov x0, 0xfffffff");
    asm("msr cntp_tval_el0, x0");
}