
#include "syscall.h"
#include "io/uart.h"

void sys_uartwrite(TRAP_FRAME* regs) {
    const char* buf = (const char*)regs->regs[0];
    U64 size = regs->regs[1];
    for(U64 i = 0; i < size; i++) {
        char c = buf[i];
        if (c == '\n') {
            uart_send_char('\r');
        }
        uart_send_char(c);
    }
    regs->regs[0] = size;
}
