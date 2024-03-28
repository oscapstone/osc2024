#include "gpio.h"
#include "aux.h"

void uart_init(void)
{
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |= 1;   // enable UART1, AUX mini uart
    *AUX_MU_CNTL = 0;   // Disable transmitter and receiver during configuration.
    *AUX_MU_LCR = 3;    // Set the data size to 8 bit.
    *AUX_MU_MCR = 0;    // Don’t need auto flow control.
    *AUX_MU_IER = 0;    // Disable interrupt because currently you don’t need interrupt.
    *AUX_MU_BAUD = 270; // Set baud rate to 115200
    *AUX_MU_IIR = 6;

    /* map UART1 to GPIO pins */
    r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15)); // gpio14, gpio15
    r |= (2 << 12) | (2 << 15);    // alt5
    *GPFSEL1 = r;

    *GPPUD = 0; // enable pins 14 and 15
    r = 150;
    while (r--)
        ;
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    r = 150;
    while (r--)
        ;
    *GPPUDCLK0 = 0;   // flush GPIO setup
    *AUX_MU_CNTL = 3; // enable Tx, Rx
}

/**
 * Send a character
 */
void uart_send(unsigned int c)
{
    /* wait until we can send */
    while (!(*AUX_MU_LSR & 0x20))
        ;
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}

/**
 * Receive a character
 */
char uart_getc()
{
    char r;
    /* wait until something is in the buffer */
    while (!(*AUX_MU_LSR & 0x01))
        ;
    /* read it and return */
    r = (char)(*AUX_MU_IO);
    /* convert carrige return to newline */
    // 將回車符 ('\r') 轉換為換行符 ('\n')。如果 r 的值為回車符，則返回換行符，否則返回 r 的原始值。
    return r == '\r' ? '\n' : r;
}

/**
 * Display a string
 */
void uart_puts(char *s)
{
    while (*s)
    {
        /* convert newline to carrige return + newline */
        if (*s == '\n')
            uart_send('\r');
        uart_send(*s++);
    }
}

void uart_2hex(unsigned int d)
{
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4)
    {
        n = (d >> c) & 0xF;
        // 如果 n 大於 9，則將其轉換為相應的 ASCII 字符（'A' 到 'F'），否則轉換為相應的數字字符（'0' 到 '9'）。這裡的 0x37 和 0x30 分別是 'A' 和 '0' 的 ASCII 碼。
        n += n > 9 ? 0x37 : 0x30;
        uart_send(n);
    }
}