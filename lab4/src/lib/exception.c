#include "exception.h"
#include "task.h"
#include "uart.h"

#define AUX_MU_IIR ((volatile unsigned int *)(0x3F215048))
#define CORE0_TIMER_IRQ_CTRL_ ((volatile unsigned int *)(0x40000040))

// priority value
#define UART_PRIORITY 4
#define TIMER_PRIORITY 3

void disable_interrupt() { asm volatile("msr DAIFSet, 0xf"); }

void enable_interrupt() { asm volatile("msr DAIFClr, 0xf"); }

void exception_handler_c()
{
    uart_puts("Exception handle\n");
    asm volatile("msr DAIFSet, 0xf\r\n");

    // read spsr_el1
    unsigned long long spsr_el1 = 0;
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr_el1));
    uart_puts("spsr_el1: ");
    uart_hex(spsr_el1);

    // read elr_el1
    unsigned long long elr_el1 = 0;
    asm volatile("mrs %0, elr_el1" : "=r"(elr_el1));
    uart_puts("elr_el1: ");
    uart_hex(elr_el1);

    // esr_el1
    unsigned long long esr_el1 = 0;
    asm volatile("mrs %0, esr_el1" : "=r"(esr_el1));
    uart_puts("esr_el1: ");
    uart_hex(esr_el1);
    uart_puts("\n");

    asm volatile("msr DAIFClr, 0xf");
}

void irq_handler_c()
{
    if (*IRQ_PENDING_1 & (1 << 29) && *CORE0_INTERRUPT_SOURCE & (1 << 8)) {
        if (*AUX_MU_IIR & 0x4) {
            uart_rx_interrupt_disable();
            add_task(UART_PRIORITY, uart_rx_handler);
            pop_task();
        }
        else if (*AUX_MU_IIR & 0x2) {
            uart_tx_interrupt_disable();
            add_task(UART_PRIORITY, uart_tx_handler);
            pop_task();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & (1 << 1)) {
        core_timer_interrupt_disable();
        add_task(TIMER_PRIORITY, core_timer_handler);
        pop_task();
        core_timer_interrupt_enable();
    }
}

void highp()
{
    uart_async_puts("high prior start\n");
    for (int i = 0; i < 100000; ++i)
        ;
    uart_async_puts("high prior end\n");
}

void lowp()
{
    uart_async_puts("low prior start\n");
    add_task(0, highp);
    uart_async_putc('\r');
    for (int i = 0; i < 100000; ++i)
        ;
    uart_async_puts("low prior end\n");
    for (int i = 0; i < 100000; ++i)
        ;
}

void test_preemption()
{
    uart_async_puts("Start Testing :\n");
    add_task(9, lowp);
    uart_async_putc('\r');
    uart_puts("End Testing\n");
}