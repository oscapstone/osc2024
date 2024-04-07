#include "exception.h"
#include "uart.h"
#include "gpio.h"

#define AUX_MU_IO ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR ((volatile unsigned int *)(MMIO_BASE + 0x00215048))

extern char read_buf[MAX_SIZE];
extern char write_buf[MAX_SIZE];
extern int read_front;
extern int read_back;
extern int write_front;
extern int write_back;

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

void uart_handler_entry()
{
    disable_interrupt(); // disable interrupt in handler first.
    if ((*AUX_MU_IIR & 0b110) == 0b100) // check if it's receiver interrupt.
    {
        if ((read_back + 1) % MAX_SIZE == read_front) // if buffer is full, disable interrupt.
        {
            disable_uart_rx_interrupt();
            return;
        }
        read_buf[read_back] = (char)*AUX_MU_IO;
        read_back = (read_back + 1) % MAX_SIZE;
    }
    else if ((*AUX_MU_IIR & 0b110) == 0b010) // check if it's transmiter interrupt
    {
        if (write_front == write_back) // if buffer is empty, disable interrupt.
        {
            disable_uart_tx_interrupt();
            return;
        }

        while (write_front != write_back)
        {
            *AUX_MU_IO = (write_buf[write_front]);
            write_front = (write_front + 1) % MAX_SIZE;
        }
        disable_uart_tx_interrupt();
    }
    enable_interrupt(); // enable interrupt in the end
}

void timer_handler_entry()
{
    unsigned long long cntpct_el0 = 0;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0)); // get timer’s current count.
    unsigned long long cntfrq_el0 = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0)); // get timer's frequency

    uart_asyn_puts("time: ");
    uart_asyn_dec(cntpct_el0 / cntfrq_el0);
    uart_asyn_puts("s\n");

    asm volatile(
        "mrs x0, cntfrq_el0\n"
        "add x0, x0, x0\n"
        "msr cntp_tval_el0, x0\n");
}