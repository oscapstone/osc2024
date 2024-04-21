#include "exception.h"
#include "mini_uart.h"

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
    uart_puts("el0_sync_router\r\n");
}

void el0_irq_64_router(){
    uart_puts("el0_irq_64_router\r\n");
}

void invalid_exception_router(unsigned long long x0){
    uart_puts("Invalid exception : 0x%x\r\n",x0);
}