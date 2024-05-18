
#include "syscall.h"
#include "io/uart.h"

void sys_uartwrite(TRAP_FRAME* regs) {
    const char* buf = (const char*)regs->regs[0];
    U64 size = regs->regs[1];
    for(U64 i = 0; i < size; i++) {
        uart_send_char(buf[i]);
    }
    regs->regs[0] = size;
}
