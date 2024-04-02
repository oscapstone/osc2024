#include "aux_reg.h"
#include "gpio.h"

void uart_send_char(unsigned int c)
{
    while (1)
    {
        asm volatile("nop");
        if (*AUX_MU_LSR_REG & 0x20) // 0x20 transmitter empty 
        {
            break;
        }
    }
    *AUX_MU_IO_REG = c; // AUX_MU_IO_REG : primary used to read/write from/to UART FIFOs
    return;
}

char uart_get_char()
{
    while (1)
    {
        if (*AUX_MU_LSR_REG &
            0x01) // bit0 : data ready , bit is set if receive FIFO hold at least 1 symbol (data occupy)
        {
            break;
        }
    }
    char c = (char)(*AUX_MU_IO_REG);
    return (c == '\r') ? '\n' : c;
    //In many systems, the carriage return \r is used to indicate the end of input
}

void uart_send_string(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        uart_send_char(str[i]);
        // uart_send((char)str[i]);
    }
    return;
}

void uart_hex(unsigned int d) //binary -> hexadecimal
{
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4)
    {
        n = (d >> c) & 0xF;
        n += (n > 9) ? 0x37 : 0x30; // 0x30 : 0 , 0x37+10 = 0x41 : A
        uart_send_char(n);
    }
}

void uart_init()
{
	// volatile: avoid non-necessary optimize from compiler
    // register: unlike auto storing data in mem, reg will suggest compiler to store data in register of CPU

    volatile register unsigned int selector;
    selector = *GPFSEL1;    // GPIO Function Select1 (support pin 10~19)
    selector &= ~(7 << 12); // clean GPIO14 (bit14~12 000)
    selector |= 2 << 12;    // set alt5 for GPIO14 (GPIO14-ALT5 : TXD1) (010 = GPIO take alt5)
    selector &= ~(7 << 15); // clean GPIO15 (bit17~15 000)
    selector |= 2 << 15;    // set alt5 for GPIO15 (GPIO15-ALT5 : RXD1) (010 = GPIO take alt5)
    *GPFSEL1 = selector;


    // Write to GPPUD to remove the control signal
    // Write to GPPUDCLK0/1 to remove the clock

    *GPPUD = 0; // control the actuation of all pull-up/down to all GPIO pin (00: disable pull-up/down)
    volatile register unsigned int t;
    t = 150; // provide the required set-up time for the control signal
    while (t--)
    {
        asm volatile("nop"); // nop will cost 1 cycle , volatile support that cmd inside it will be moved
    }
    
    *GPPUDCLK0 = (1 << 14) | (1 << 15); // assert clock on line 14,15
    t = 150;                            // provide the required hold time for the control signal
    while (t--)
    {
        asm volatile("nop");
    }

    *GPPUDCLK0 = 0;

    *AUX_ENABLES |= 1;      // enable mini uart , can access to its registers
    *AUX_MU_CNTL_REG = 0;   // disable transmitter(TX) and receiver(RX) (bit1:transimitter , bit0:receiver)
    *AUX_MU_LCR_REG = 3;    // set the data size to 8bit (00:7bit mode  11:8bit mode)
    *AUX_MU_MCR_REG = 0;    // don't need auto flow control (control by the bit1)
    *AUX_MU_IER_REG = 0;    // disable receive interrupt
    *AUX_MU_BAUD_REG = 270; // set baud rate to 115200, 


    *AUX_MU_CNTL_REG = 3; // enable transmitter and receiver
    return;
}
