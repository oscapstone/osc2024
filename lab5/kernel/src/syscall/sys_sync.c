
#include "syscall.h"
#include "io/uart.h"
#include "peripherals/irq.h"
#include "fs/fs.h"


void sys_sync(TRAP_FRAME *regs) {
    fs_sync();
}
