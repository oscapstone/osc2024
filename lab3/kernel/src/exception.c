#include "exception.h"
#include "mini_uart.h"
#include "utils.h"

void print_currentEL(){
    unsigned long long currentEL;
    __asm__ __volatile__("mrs %0, currentEL\n\t" : "=r"(currentEL));
    
    switch (currentEL) {
        case 0:
            uart_puts("Current EL: EL0\r\n");
            break;
        case 4:
            uart_puts("Current EL: EL1\r\n");
            break;
        case 8:
            uart_puts("Current EL: EL2\r\n");
            break;
        case 12:
            uart_puts("Current EL: EL3\r\n");
            break;
        default:
            uart_puts("Current EL: UNKNOWN\r\n");
            break;
    }
}

void el1h_irq_router(){
    uart_puts("el1h_irq_router\r\n");
}

void el0_sync_router(){
    // Print the content of spsr_el1, elr_el1, and esr_el1 in the exception handler.
    unsigned long long spsr_el1;
    unsigned long long elr_el1;
    unsigned long long esr_el1;
    __asm__ __volatile__("mrs %0, SPSR_EL1\n\t" : "=r"(spsr_el1)); // EL1 configuration, spsr_el1[9:6]=4b0 to enable interrupt
    __asm__ __volatile__("mrs %0, ELR_EL1\n\t"  : "=r"(elr_el1)); // ELR_EL1 holds the address if return to EL1
    __asm__ __volatile__("mrs %0, ESR_EL1\n\t"  : "=r"(esr_el1)); // ESR_EL1 holds symdrome information of exception, to know why the exception happens.

    char buf[VSPRINT_MAX_BUF_SIZE];
    sprintf(buf, "[Exception][el0_sync] spsr_el1: 0x%x | elr_el1: 0x%x | esr_el1 : 0x%x\r\n", spsr_el1, elr_el1, esr_el1);
    uart_puts(buf);
}

void el0_irq_64_router(){
    uart_puts("el0_irq_64_router\r\n");
}

void invalid_exception_router(unsigned long long x0){
    uart_puts("Invalid exception : 0x%x\r\n",x0);
}