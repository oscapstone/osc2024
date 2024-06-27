#include <stdint.h>

#include "irq.h"
#include "proc.h"
#include "syscall.h"
#include "traps.h"
#include "uart.h"

void print_registers(uint64_t elr, uint64_t esr, uint64_t spsr)
{
    uart_puts("spsr_el1: ");
    uart_hex(spsr);
    uart_puts("\n elr_el1: ");
    uart_hex(elr);
    uart_puts("\n esr_el1: ");
    uart_hex(esr);
    uart_puts("\n\n");
}

void exception_entry(pt_regs *tf)
{
    unsigned long elr, esr, spsr;
    asm volatile("mrs %0, elr_el1" : "=r"(elr));
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr));
    if (esr != 0x56000000) {
        print_registers(elr, esr, spsr);
        while (1)
            ;
    }

    enable_interrupt();
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
        tf->elr_el1 = 0x0;
        tf->sp_el0 = 0xFFFFFFFFF000;
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
    case 8:
        sys_signal(tf->x0, (void (*)())tf->x1);
        break;
    case 9:
        sys_sigkill(tf->x0, tf->x1);
        break;
    case 11:
        tf->x0 = sys_open((const char *)tf->x0, tf->x1);
        break;
    case 12:
        tf->x0 = sys_close(tf->x0);
        break;
    case 13:
        tf->x0 = sys_write(tf->x0, (const void *)tf->x1, tf->x2);
        break;
    case 14:
        tf->x0 = sys_read(tf->x0, (void *)tf->x1, tf->x2);
        break;
    case 15:
        tf->x0 = sys_mkdir((const char *)tf->x0, tf->x1);
        break;
    case 16:
        tf->x0 = sys_mount((const char *)tf->x0, (const char *)tf->x1,
                           (const char *)tf->x2, tf->x3, (const void *)tf->x4);
        break;
    case 17:
        tf->x0 = sys_chdir((const char *)tf->x0);
        break;
    case 18:
        tf->x0 = sys_lseek64(tf->x0, tf->x1, tf->x2);
        break;
    case 19:
        tf->x0 = sys_ioctl(tf->x0, tf->x1, (void *)tf->x2);
        break;
    case 139:
        sys_sigreturn(tf);
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
