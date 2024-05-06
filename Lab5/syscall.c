#include "uart.h"
#include "thread.h"

//get elsel1, elrel1 and sp (Trapframe * = sp)

void async_exception_entry(){
    unsigned long el;
    asm volatile ("mrs %0, CurrentEL" : "=r" (el));
    el = el >> 2;
    uart_puts("Booted! Current EL: ");
    uart_send('0' + el);
    uart_puts("\n");
}