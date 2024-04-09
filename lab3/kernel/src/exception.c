#include "uart.h"
#include "exception.h"
#include "timer.h"

void disable_interrupt(void) { //ok
    asm volatile("msr DAIFSet, 0xf\r\n");
}

void el0_irq_entry() { //ok
    print_boot_time();
    //In the basic part, you only need to enable interrupt in EL0
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

    // enable_interrupt
    asm volatile("msr DAIFCLr, 0xf\r\n");

}