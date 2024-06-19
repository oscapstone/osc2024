#include "include/interrupt.h"
#include "include/uart.h"

#define CORE0_INTERRUPT_SOURCE	((volatile unsigned int*)(0x40000060))
// print_boot_timer
// cntfrq_el0 = 62500000
int print_boot_timer(){
    uart_puts("Interrupt occurs!\n");
    unsigned long long freq, ticks;
    asm volatile(
    "mrs %0, cntfrq_el0;"
    "mrs %1, cntpct_el0;"
        : "=r" (freq), "=r" (ticks)
    );
    // set up the next timer
    asm volatile (
      "msr cntp_tval_el0, %0" 
        : 
        : "r" (freq * 2)
    );
    unsigned int time = (unsigned int)(ticks/freq);
    uart_hex(time);
    uart_puts(" seconds elapsed\n");
  return 0;
}


void interrupt_entry(){
    //asm volatile("msr DAIFSet, 0xf");
    // int irq_pending1 = *IRQ_PENDING_1;
    int core0_irq = *CORE0_INTERRUPT_SOURCE;
    int iir = *AUX_MU_IIR;
    if (core0_irq & (1 << 1)){
        print_boot_timer();
    }
    else{
        // P13, Receiver holds valid byte
        if ((iir & 0x06) == 0x04){
            uart_read_handler();
        }
        // P13, Transmit holding register empty
        //else if((iir & 0x06) == 0x02){
        else{
            uart_write_handler();
        }
    }
    return;
}
