#include "exception.h"
#include "sched.h"
#include "signal.h"
#include "syscall.h"
#include "task.h"
#include "uart.h"

#define AUX_MU_IIR ((volatile unsigned int *)(0x3F215048))
#define CORE0_TIMER_IRQ_CTRL_ ((volatile unsigned int *)(0x40000040))

// priority value
#define UART_PRIORITY 4
#define TIMER_PRIORITY 3

extern int thread_sched_init_finished;

void el1_interrupt_enable() { asm volatile("msr DAIFClr, 0xf"); }

void el1_interrupt_disable() { asm volatile("msr DAIFSet, 0xf"); }

static unsigned long long lock_counter = 0;
void lock()
{
    asm volatile("msr DAIFSet, 0xf");
    lock_counter++;
}

void unlock()
{
    if (--lock_counter == 0)
        asm volatile("msr DAIFClr, 0xf");
}

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

void irq_handler_c(trapframe_t *tf)
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

        if (thread_sched_init_finished)
            schedule();
    }

    if ((tf->spsr_el1 & 0b1100) == 0) {
        check_signal(tf);
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
}

void el0_sync_router(trapframe_t *tf)
{
    // interrupt enable
    asm volatile("msr DAIFClr, 0xf");

    unsigned long long syscall_no = tf->regs[8];
    // uart_printf("syscall_no: %d\n", syscall_no);
    switch (syscall_no) {
    case 0:
        getpid(tf);
        break;
    case 1:
        uart_read(tf, (char *)tf->regs[0], tf->regs[1]);
        break;
    case 2:
        uart_write(tf, (char *)tf->regs[0], tf->regs[1]);
        break;
    case 3:
        exec(tf, (char *)tf->regs[0], (char **)tf->regs[1]);
        break;
    case 4:
        fork(tf);
        break;
    case 5:
        exit(tf);
        break;
    case 6:
        mbox_call(tf, (unsigned char)tf->regs[0], (unsigned int *)tf->regs[1]);
        break;
    case 7:
        kill(tf, (int)tf->regs[0]);
        break;
    case 8:
        // uart_printf("syscall_no: %d\n", syscall_no);
        signal_register(tf->regs[0], (void (*)())tf->regs[1]);
        break;
    case 9:
        // uart_printf("syscall_no: %d\n", syscall_no);
        signal_kill(tf->regs[0], tf->regs[1]);
        break;
    case 50:
        // uart_printf("syscall_no: %d\n", syscall_no);
        signal_return(tf);
        break;
    default:
        break;
    }
}

void el0_irq_router(trapframe_t *tf)
{
    lock();
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

        if (thread_sched_init_finished)
            schedule();
    }

    if ((tf->spsr_el1 & 0b1100) == 0) {
        check_signal(tf);
    }
}

void invalid_op_handler()
{
    uart_printf("Invalid operation handler\n");
    while (1)
        ;
}