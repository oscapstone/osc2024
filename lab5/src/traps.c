#include <stdint.h>

#include "syscall.h"
#include "traps.h"
#include "uart.h"

void print_registers(uint64_t elr, uint64_t esr, uint64_t spsr)
{
    // Print spsr_el1
    uart_puts("spsr_el1: ");
    uart_hex(spsr);
    uart_putc('\n');

    // Print elr_el1
    uart_puts(" elr_el1: ");
    uart_hex(elr);
    uart_putc('\n');

    // Print esr_el1
    uart_puts(" esr_el1: ");
    uart_hex(esr);
    uart_puts("\n\n");
}

void exception_entry(trap_frame *tf)
{
    switch (tf->x8) {
    case 0:
        tf->x0 = sys_getpid();
        break;
    case 1:
        tf->x0 = sys_uart_read((char *)tf->x0, tf->x1);
        break;
    case 2:
        tf->x0 = sys_uart_write((const char *)tf->x0, tf->x1);
        break;
    case 3:
        tf->x0 = sys_exec((const char *)tf->x0, (char *const *)tf->x1);
        break;
    case 4:
        tf->x0 = sys_fork(tf);
        break;
    case 5:
        sys_exit();
        break;
    case 6:
        tf->x0 = sys_mbox_call(tf->x0, (unsigned int *)tf->x1);
        break;
    case 7:
        sys_kill(tf->x0);
        break;
    default:
        uart_puts("[ERROR] Invalid system call\n");
    }
}

void invalid_entry(uint64_t elr, uint64_t esr, uint64_t spsr)
{
    uart_puts("[ERROR] The exception handler is not implemented\n");
    print_registers(elr, esr, spsr);
    while (1)
        ;
}