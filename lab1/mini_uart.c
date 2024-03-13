#include "headers/mini_uart.h"
#include "headers/gpio.h"

#define AUX_OFFSET BUS2PHY(0x7E210000)

#define AUX_IRQ             (( volatile unsigned int *)(AUX_OFFSET + 0x5000))
#define AUX_ENABLES         (( volatile unsigned int *)(AUX_OFFSET + 0x5004))

#define AUX_MU_IO_REG       (( volatile unsigned int *)(AUX_OFFSET + 0x5040))
#define AUX_MU_IER_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x5044))
#define AUX_MU_IIR_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x5048))
#define AUX_MU_LCR_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x504C))
#define AUX_MU_MCR_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x5050))
#define AUX_MU_LSR_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x5054))
#define AUX_MU_MSR_REG      (( volatile unsigned int *)(AUX_OFFSET + 0x5058))
#define AUX_MU_SCRATCH      (( volatile unsigned int *)(AUX_OFFSET + 0x505C))
#define AUX_MU_CNTL_REG     (( volatile unsigned int *)(AUX_OFFSET + 0x5060))
#define AUX_MU_STAT_REG     (( volatile unsigned int *)(AUX_OFFSET + 0x5064))
#define AUX_MU_BAUD_REG     (( volatile unsigned int *)(AUX_OFFSET + 0x5068))

#define AUX_SPI1_CNTL0_REG  (( volatile unsigned int *)(AUX_OFFSET + 0x5080))
#define AUX_SPI1_CNTL1_REG  (( volatile unsigned int *)(AUX_OFFSET + 0x5084))
#define AUX_SPI1_STAT_REG   (( volatile unsigned int *)(AUX_OFFSET + 0x5088))

#define AUX_SPI1_IO_REG     (( volatile unsigned int *)(AUX_OFFSET + 0x5090))
#define AUX_SPI1_PEEK_REG   (( volatile unsigned int *)(AUX_OFFSET + 0x5094))

#define AUX_SPI2_CNTL0_REG  (( volatile unsigned int *)(AUX_OFFSET + 0x50C0))
#define AUX_SPI2_CNTL1_REG  (( volatile unsigned int *)(AUX_OFFSET + 0x50C4))
#define AUX_SPI2_STAT_REG   (( volatile unsigned int *)(AUX_OFFSET + 0x50C8))

#define AUX_SPI2_IO_REG     (( volatile unsigned int *)(AUX_OFFSET + 0x50D0))
#define AUX_SPI2_PEEK_REG   (( volatile unsigned int *)(AUX_OFFSET + 0x50D4))

void delay( volatile unsigned int time)
{
    while ( time--)
    {
        asm volatile("nop");
    }// while
    return;
}

void mini_uart_init()
{
    // connecting to GPIO pins 14 & 15
    // only using 3 bits
    register unsigned int reg = *GPFSEL1;
    reg &= ~(( 0xF >> 1) << 12); // clean them
    reg &= ~(( 0xF >> 1) << 15);
    reg |= ( 0x2 << 12); // set them
    reg |= ( 0x2 << 15);
    *GPFSEL1 = reg; // write back

    // commit the signal
    // need to disable GPIO pull-up/down functions
    *GPPUD = 0;
    delay( 150);
    *GPPUDCLK0 = ( 1 << 14) | ( 1 << 15); // select the pins
    delay( 150);
    *GPPUDCLK0 = 0; // reset it

    // set uart enable
    *AUX_ENABLES |= 1;

    *AUX_MU_CNTL_REG = 0;
    *AUX_MU_IER_REG = 0;
    *AUX_MU_LCR_REG = 3;
    *AUX_MU_MCR_REG = 0;
    *AUX_MU_BAUD_REG = 270;
    *AUX_MU_IIR_REG = 6;
    *AUX_MU_CNTL_REG = 3;

    return;
}

char mini_uart_getc()
{
    // check the status of uart
    while ( 1)
    {
        if (( *AUX_MU_LSR_REG) & 0x01)
        {
            break;
        }// if
    }// while

    char recv = (char)( *AUX_MU_IO_REG);

    // convert the carrige return
    return recv == '\r' ? '\n': recv;
}

void mini_uart_putc( char c)
{
    // check the status of uart
    while ( 1)
    {
        if (( *AUX_MU_LSR_REG) & ( 0x01 << 5))
        {
            break;
        }// if
    }// while

    *AUX_MU_IO_REG = (unsigned int)c;
    return;
}

void mini_uart_puts( char *s)
{
    while ( *s != '\0')
    {
        mini_uart_putc( *s);
        s += 1;
    }// while
    
    return;
}

void mini_uart_puthexint( unsigned int number)
{
    char next;
    // change endianess
    for ( int i = 28; i >= 0; i -= 4)
    {
        // get number in little endian
        next = (( number >> i) & 0x0F);

        next = next <= 0x9 ? next + '0': next + 'A' - 0xA;

        mini_uart_putc( next);
    }// for i
    
    return;
}
