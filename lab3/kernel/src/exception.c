#include "gpio.h"
#include "uart.h"
#include "task.h"
#include "timer.h"
#include "exception.h"

#define AUX_MU_IO ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LSR ((volatile unsigned int *)(MMIO_BASE + 0x00215054))

#define CORE0_INTERRUPT_SOURCE (volatile unsigned int *)0x40000060

#define TX_PRIORITY 4
#define RX_PRIORITY 4
#define TIMER_PRIORITY 0

extern char read_buf[MAX_SIZE];
extern char write_buf[MAX_SIZE];
extern int read_front;
extern int read_back;
extern int write_front;
extern int write_back;

extern task_heap *task_hp;
extern timer_heap *timer_hp;

void enable_interrupt()
{
    asm volatile("msr DAIFClr, 0xf");
}

void disable_interrupt()
{
    asm volatile("msr DAIFSet, 0xf");
}

void exception_entry()
{
    uart_puts("In Exception handle\n");

    unsigned long long spsr_el1 = 0;
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr_el1));
    uart_puts("spsr_el1: ");
    uart_hex_lower_case(spsr_el1);
    uart_puts("\n");

    unsigned long long elr_el1 = 0;
    asm volatile("mrs %0, elr_el1" : "=r"(elr_el1));
    uart_puts("elr_el1: ");
    uart_hex_lower_case(elr_el1);
    uart_puts("\n");

    unsigned long long esr_el1 = 0;
    asm volatile("mrs %0, esr_el1" : "=r"(esr_el1));
    uart_puts("esr_el1: ");
    uart_hex_lower_case(esr_el1);
    uart_puts("\n");

    return;
}

void irq_handler_entry()
{
    if (*CORE0_INTERRUPT_SOURCE & (1 << 8)) // interrupt is from GPU
    {
        if (*AUX_MU_IIR & 0b100) // check if it's receiver interrupt.
        {
            disable_uart_rx_interrupt(); // masks the device’s interrupt line
            task t;
            t.callback = rx_task;
            t.priority = RX_PRIORITY;

            disable_interrupt();
            task_heap_insert(task_hp, t);
            enable_interrupt();

            do_task();
        }
        else if (*AUX_MU_IIR & 0b010) // check if it's transmiter interrupt
        {
            disable_uart_tx_interrupt(); // masks the device’s interrupt line
            task t;
            t.callback = tx_task;
            t.priority = TX_PRIORITY;

            disable_interrupt();
            task_heap_insert(task_hp, t);
            enable_interrupt();

            do_task();
        }
    }
    else if (*CORE0_INTERRUPT_SOURCE & (1 << 1)) // interrupt is from CNTPNSIRQ
    {
        core_timer_disable(); // masks the device’s interrupt line
        task t;
        t.callback = timer_task;
        t.priority = TIMER_PRIORITY;

        disable_interrupt();
        task_heap_insert(task_hp, t);
        enable_interrupt();

        do_task();
    }
}

void rx_task()
{
    if ((read_back + 1) % MAX_SIZE == read_front) // if buffer is full, then return.
        return;

    while ((*AUX_MU_LSR & 1) && ((read_back + 1) % MAX_SIZE != read_front))
    {
        read_buf[read_back] = uart_read();
        read_back = (read_back + 1) % MAX_SIZE;
    }
}

void tx_task()
{
    if (write_front == write_back) // if buffer is empty, then return.
        return;

    while (write_front != write_back)
    {
        uart_write(write_buf[write_front]);
        write_front = (write_front + 1) % MAX_SIZE;
    }
}

void timer_task()
{
    timer t = timer_heap_extractMin(timer_hp);
    if (t.expire == -1) // the timer heap is empty
        return;

    t.callback(t.data, t.executed_time);

    if (timer_hp->size > 0)
    {
        set_min_expire();
        core_timer_enable();
    }
    else
        core_timer_disable();
}

void p0_task()
{
    uart_puts("p0_task start\n");

    task t;
    t.callback = p3_task;
    t.priority = 3;
    disable_interrupt();
    task_heap_insert(task_hp, t);
    enable_interrupt();
    uart_asyn_write('\n'); // trigger interrupt
    int count = 100000;    // wait for interrupt
    while (count--)
        asm volatile("nop\n");

    uart_puts("p0_task end\n");
}

void p1_task()
{
    uart_puts("p1_task start\n");

    task t;
    t.callback = p0_task;
    t.priority = 0;
    disable_interrupt();
    task_heap_insert(task_hp, t);
    enable_interrupt();
    uart_asyn_write('\n'); // trigger interrupt

    int count = 100000; // wait for interrupt
    while (count--)
        asm volatile("nop\n");

    uart_puts("p1_task end\n");
}

void p2_task()
{
    uart_puts("p2_task start\n");

    task t;
    t.callback = p1_task;
    t.priority = 1;
    disable_interrupt();
    task_heap_insert(task_hp, t);
    enable_interrupt();
    uart_asyn_write('\n'); // trigger interrupt

    int count = 100000; // wait for interrupt
    while (count--)
        asm volatile("nop\n");

    uart_puts("p2_task end\n");
}

void p3_task()
{
    uart_puts("p3_task start\n");
    uart_puts("p3_task end\n");
}

void test_preemption()
{
    task t;
    t.callback = p2_task;
    t.priority = 2;
    disable_interrupt();
    task_heap_insert(task_hp, t);
    enable_interrupt();

    uart_asyn_write('\n'); // trigger interrupt
    int count = 100000;    // wait for interrupt
    while (count--)
        asm volatile("nop\n");
}