#include "bsp/traps.h"
#include <bsp/irq.h>
#include <bsp/syscall.h>
#include <bsp/uart.h>
#include <kernel/io.h>
#include <kernel/sched.h>
#include <kernel/timer.h>

void print_registers(uint64_t elr, uint64_t esr, uint64_t spsr) {
    print_string("spsr_el1: ");
    print_h(spsr);
    print_string("\n elr_el1: ");
    print_h(elr);
    print_string("\n esr_el1: ");
    print_h(esr);
    print_string("\n\n");
}

void exception_entry(trap_frame *tf) {
    unsigned long elr, esr, spsr;
    asm volatile("mrs %0, elr_el1" : "=r"(elr));
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr));
    if (esr != 0x56000000) {
        print_registers(elr, esr, spsr);
        while (1);
    }

    enable_irq();
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
            tf->elr_el1 = get_current()->context.lr;
            tf->sp_el0 = (unsigned long)get_current()->user_stack + STACK_SIZE;
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
        case 139:
            sys_sigreturn(tf);
            break;
        default:
            print_string("[ERROR] Invalid system call\n");
    }
}

void invalid_entry(uint64_t elr, uint64_t esr, uint64_t spsr) {
    print_string("[ERROR] The exception handler is not implemented\n");
    print_registers(elr, esr, spsr);
    while (1);
}

void irq_entry(trap_frame *tf) {
    
    disable_irq();

    if (is_core_timer_irq()) {
        // Schedule processes
        if (get_current() != get_current()->next) {
            schedule();
        }
        core_timer_disable();
        timer_irq_handler();
    } else if (is_gpu_irq()) {
        if (is_aux_irq()) {
            if (is_uart_rx_irq()) {
                uart_disable_rx_interrupt();
                uart_rx_irq_handler();
            } else if (is_uart_tx_irq()) {
                uart_disable_tx_interrupt();
                uart_tx_irq_handler();
            }
        }
    } else {
        print_string("\r\nUnknown irq");
    }

    do_signal(tf);
}
