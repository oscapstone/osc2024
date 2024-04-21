#include "exception.h"
#include "mini_uart.h"

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