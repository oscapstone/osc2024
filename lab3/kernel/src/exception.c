#include "uart.h"
#include "exception.h"
#include "timer.h"
#include "tasklist.h"

void disable_interrupt(void) {
    asm volatile("msr DAIFSet, 0xf\r\n");
}

void enable_interrupt(void) {
    asm volatile("msr DAIFClr, 0xf\r\n");
}

void el0_irq_entry(void) {
    print_boot_time();
    //In the basic part, you only need to enable interrupt in EL0
}

void el1h_irq_entry(void) {
    disable_interrupt();
    //IRQ_PENDING_1 掌握了中斷的狀態，我們可以利用這個暫存器去檢查 interrupt 是由哪個 timer 產生的

    if ((*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT) && (*CORE0_IRQ_SOURCE & IRQ_SOURCE_GPU)) { //IRQ_SOURCE_GPU => One or more bits set in pending register 1
        //ensure ( interrupt and aux )  and ( gpu interrupt )  // bit [8][9] meansThere are some interrupts pending which you don't know about. They are in pending register 1 /2." 
        async_uart_handler();
    } else if (*CORE0_IRQ_SOURCE & IRQ_SOURCE_CNTPNSIRQ) {
        remove_core_timer();
    }

    enable_interrupt();
}

void exception_entry(void) {
    disable_interrupt();

    unsigned long elr, esr, spsr;

    asm volatile("mrs %0, spsr_el1" :"=r"(spsr) );//Program Status Register
    asm volatile("mrs %0, elr_el1"  :"=r"(elr)  );//Exception Link Register  return address
    asm volatile("mrs %0, esr_el1"  :"=r"(esr)  );//Exception Syndrome Register 異常綜合表徵寄存器esr_eln包含的異常信息用以異常處理程序確定異常原因
    
    uart_puts("spsr_el1:\t");
    uart_hex(spsr);
    uart_puts("\r\n");

    uart_puts("elr_el1:\t");
    uart_hex(elr);
    uart_puts("\r\n");

    uart_puts("esr_el1:\t");
    uart_hex(esr);
    uart_puts("\r\n");

    uart_puts("\r\n");

    enable_interrupt();
}
