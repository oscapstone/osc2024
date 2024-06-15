#include "rpi_mini_uart.h"
#include "rpi_gpio.h"
#include "rpi_irq.h"
#include "timer.h"
#include "utility.h"
// implement first in first out buffer with a read index and a write index
char uart_tx_buffer[0x100] = {};
int uart_tx_buffer_widx = 0; // write buffer written index
int uart_tx_buffer_ridx = 0; // read buffer read index
char uart_rx_buffer[0x100] = {};
int uart_rx_buffer_widx = 0; // read buffer written index
int uart_rx_buffer_ridx = 0; // read buffer read index
//////////////////////////////////////////////////////////////////////////

void uart_init(void)
{
    register int selector;

    *AUX_ENABLES |= 1;    // Enable mini uart (this also enables access to its registers)
    *AUX_MU_CNTL_REG = 0; // Disable auto flow control and disable receiver and transmitter (for now)

    selector = *(GPFSEL1);
    selector &= ~(7 << 12); // clean gpio14
    selector |= 2 << 12;    // set alt5 for gpio14
    selector &= ~(7 << 15); // clean gpio15
    selector |= 2 << 15;    // set alt5 for gpio 15
    *GPFSEL1 = selector;

    *GPPUD = 0;
    selector = 150;
    while (selector--)
    {
        asm volatile("nop");
    }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    selector = 150;
    while (selector--)
    {
        asm volatile("nop");
    }
    *GPPUDCLK0 = 0;

    *AUX_MU_IER_REG = 0;    // Disable receive and transmit interrupts
    *AUX_MU_LCR_REG = 3;    // Enable 8 bit mode
    *AUX_MU_MCR_REG = 0;    // Set RTS line to be always high
    *AUX_MU_BAUD_REG = 270; // Set baud rate to 115200

    *AUX_MU_CNTL_REG = 3; // Finally, enable transmitter and receiver
}

void put_currentEL(void)
{
    unsigned long long currentEL;
    __asm__ __volatile__("mrs %0, currentEL\n\t" : "=r"(currentEL)); //
    put_int(currentEL);
    uart_puts("\n");
};

char uart_recv()
{
    char r;
    while (!(*AUX_MU_LSR_REG & 0x01))
    {
    };
    r = (char)(*AUX_MU_IO_REG);

    return r;
}

void uart_send(unsigned int c)
{
    while (!(*AUX_MU_LSR_REG & 0x20))
    {
    };
    *AUX_MU_IO_REG = c;
}

void uart_puts(char *str)
{
    while (*str)
        uart_send(*str++);
}
void uart_get()
{
    char r;
    while (!(*AUX_MU_LSR_REG & 0x01))
    {
    };
    r = (char)(*AUX_MU_IO_REG);
    return r;
}

void put_int(int num)
{
    // Handle the case when the number is 0
    if (num == 0)
    {
        uart_send('0');
        return;
    }

    // Temporary array to store the reversed digits as characters
    char temp[12]; // Assuming int can have at most 10 digits
    int idx = 0;

    // Handle negative numbers
    if (num < 0)
    {
        uart_send('-');
        num = -num;
    }

    // Convert the number to characters and store in the temporary array in reverse order
    while (num > 0)
    {
        temp[idx++] = (char)(num % 10 + '0');
        num /= 10;
    }

    // Reverse output the character digits
    while (idx > 0)
    {
        uart_send(temp[--idx]);
    }
}
void uart_hex(unsigned int d)
{
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4)
    {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_send(n);
    }
}

///////////// following new////////////////////////////////////////////////////
// uart_async_getc read from buffer
// uart_r_irq_handler write to buffer then output
char uart_async_getc()
{
    *AUX_MU_IER_REG |= 1; // Enable receive interrupts. [0]bit 負責接收interrupt的開關

    lock();
    while (uart_rx_buffer_ridx == uart_rx_buffer_widx)
    {
        unlock();
        *AUX_MU_IER_REG |= 1; // Enable receive interrupts.
        lock();
    }
    // uart_puts("[uart_async_getc] \n");
    char r = uart_rx_buffer[uart_rx_buffer_ridx++];
    if (uart_rx_buffer_ridx >= 0x100)
        uart_rx_buffer_ridx = 0;
    unlock();
    return r;
}

// uart_async_putc writes to buffer
// uart_w_irq_handler read from buffer then output
void uart_async_putc(char c)
{
    lock();
    // if buffer full, wait for uart_w_irq_handler
    while ((uart_tx_buffer_widx + 1) % 0x100 == uart_tx_buffer_ridx)
    {
        unlock();
        *AUX_MU_IER_REG |= 2; // enable write interrupt
        lock();
    }
    // uart_puts("[uart_async_putc] \n");
    uart_tx_buffer[uart_tx_buffer_widx++] = c;
    if (uart_tx_buffer_widx >= 0x100)
        uart_tx_buffer_widx = 0; // cycle pointer
    unlock();
    *AUX_MU_IER_REG |= 2; // enable write interrupt
}

char getchar()
{
    char c = uart_async_getc();
    return c == '\r' ? '\n' : c;
}

void putchar(char c)
{
    uart_async_putc(c);
}

void puts(const char *s)
{
    while (*s)
        putchar(*s++);
}

int is_uart_rx_buffer_full()
{
    return (uart_rx_buffer_widx + 1) % 0x100 == uart_rx_buffer_ridx;
}

int is_uart_tx_buffer_full()
{
    return (uart_tx_buffer_widx + 1) % 0x100 == uart_tx_buffer_ridx;
}

// AUX_MU_IER_REG -> BCM2837-ARM-Peripherals.pdf - Pg.12
void uart_interrupt_enable()
{
    *AUX_MU_IER_REG |= 1;      // enable read interrupt
    *AUX_MU_IER_REG |= 2;      // enable write interrupt
    *ENABLE_IRQS_1 |= 1 << 29; // Pg.112
}

void uart_interrupt_disable()
{
    *AUX_MU_IER_REG &= ~(1); // disable read interrupt
    *AUX_MU_IER_REG &= ~(2); // disable write interrupt
}

void uart_r_irq_handler()
{
    lock();
    if ((uart_rx_buffer_widx + 1) % 0x100 == uart_rx_buffer_ridx)
    {
        *AUX_MU_IER_REG &= ~(1); // disable read interrupt
        unlock();
        return;
    }
    unlock();
    uart_rx_buffer[uart_rx_buffer_widx] = uart_recv();
    lock();
    // uart_puts("[uart r irq handler] \n");
    uart_rx_buffer_widx++;
    if (uart_rx_buffer_widx >= 0x100)
        uart_rx_buffer_widx = 0;
    *AUX_MU_IER_REG |= 1;
    unlock();
}
int timerup = 0;

void uart_w_irq_handler()
{
    // for Lab 3 Premmptive
	// unlock();
    // if (*CORE0_TIMER_IRQ_CTRL &= 1)
    // {
    //     for (int i = 0; i < 1000000000; i++)
    //     {
    //     };
    // };
    // lock();
    lock();
    if (uart_tx_buffer_ridx == uart_tx_buffer_widx)
    {
        *AUX_MU_IER_REG &= ~(2); // disable write interrupt
        unlock();
        return; // buffer empty
    }
    unlock();
    uart_send(uart_tx_buffer[uart_tx_buffer_ridx]);
    lock();
    // uart_puts("[uart_w_irq_handler()] \n");
    uart_tx_buffer_ridx++;
    if (uart_tx_buffer_ridx >= 0x100)
        uart_tx_buffer_ridx = 0;
    *AUX_MU_IER_REG |= 2; // enable write interrupt
    unlock();
}

void uart_flush_FIFO()
{
    // https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf Pg.13
    // Both bits always read as 1 as the FIFOs are always enabled

    // The AUX_MU_IIR_REG register shows the interrupt status.
    // It also has two FIFO enable status bits and (when writing) FIFO clear bits.
    *AUX_MU_IIR_REG |= 6;
    // On write:
    // Writing with bit 1 set will clear the receive FIFO
    // Writing with bit 2 set will clear the transmit FIFO
}
