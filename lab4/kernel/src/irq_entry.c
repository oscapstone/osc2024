#include <kernel/bsp_port/irq.h>
#include <kernel/bsp_port/uart.h>
#include <kernel/io.h>
#include <kernel/irq_entry.h>
#include <kernel/task_queue.h>
#include <kernel/timer.h>

static void show_irq_debug_msg(int type, unsigned long spsr, unsigned long elr,
                               unsigned long esr) {
    print_string("\r\n");
    print_string(get_exception_message(type));
    print_string(", SPSR: ");
    print_h(spsr);
    print_string(", ELR: ");
    print_h(elr);
    print_string(", ESR: ");
    print_h(esr);
}

void el1_irq_entry(int type, unsigned long spsr, unsigned long elr,
                   unsigned long esr) {
    #ifdef IRQ_DEBUG
        uart_puts("IRQ count: ");
        uart_hex(count);
        uart_puts("\n");
        uart_puts("IRQ CORE0_IRQ_SOURCE: ");
        uart_hex(get_irq());
        uart_puts("\n");
        print_string("\n[el1_irq_entry] ");
        show_irq_debug_msg(type, spsr, elr, esr);
    #endif

    el1_disable_interrupt();

    if (is_core_timer_irq()) {
        core_timer_disable();
        add_task(timer_irq_handler, EL1_IRQ_TIMER_PRIORITY);
    } else if (is_gpu_irq()) {
        if (is_aux_irq()) {
            if (is_uart_rx_irq()) {
                uart_disable_rx_interrupt();
                add_task(uart_rx_irq_handler, EL1_IRQ_UART_PRIORITY);
            } else if (is_uart_tx_irq()) {
                uart_disable_tx_interrupt();
                add_task(uart_tx_irq_handler, EL1_IRQ_UART_PRIORITY);
            }
        }
    } else {
        print_string("\r\nUnknown irq");
    }
    el1_enable_interrupt();

    run_task();
}

void default_exception_entry(int type, unsigned long spsr, unsigned long elr,
                             unsigned long esr) {
    show_irq_debug_msg(type, spsr, elr, esr);
    while (1);
}

void el0_irq_entry(int type, unsigned long spsr, unsigned long elr,
                   unsigned long esr) {
    // el1_disable_interrupt();
    show_irq_debug_msg(type, spsr, elr, esr);
    // el1_enable_interrupt();
}

void svc_exception_entry(int type, unsigned long spsr, unsigned long elr,
                         unsigned long esr) {
    // el1_disable_interrupt();
    show_irq_debug_msg(type, spsr, elr, esr);
    // el1_enable_interrupt();
}
