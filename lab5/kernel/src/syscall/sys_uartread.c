
#include "syscall.h"
#include "io/uart.h"
#include "peripherals/irq.h"

void sys_uartread(TRAP_FRAME *regs) {
    char* buf = (char*)regs->regs[0];
    U64 size = (U64)regs->regs[1];
    for (U64 i = 0; i < size; i++) {
        while (uart_async_empty()) {
            asm volatile("nop");
            //task_schedule();            // just schedule this task
        }
        char c = uart_async_get_char();
        buf[i] = c;
    }
    regs->regs[0] = size;
}
