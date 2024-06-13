#include "../include/gpio.h"
#include "../include/uart.h"
#include "../include/stdint.h"

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init()
{
    register unsigned int reg;

    /* initialize UART */
    *AUX_ENABLE     |= 1;       /* enable mini UART */
    *AUX_MU_CNTL     = 0;       /* Disable transmitter and receiver during configuration. */

    *AUX_MU_IER      = 0;       /* Disable interrupt */
    *AUX_MU_LCR      = 3;       /* Set the data size to 8 bit. */
    *AUX_MU_MCR      = 0;       /* Don’t need auto flow control. */
    *AUX_MU_BAUD     = 270;     /* 115200 baud */
    *AUX_MU_IIR      = 6;       /* No FIFO */
    // *AUX_MU_IIR      = 0xc6;       /* No FIFO */

    /* map UART1 to GPIO pins */
    // GPFSEL1 register controls the function of GPIO pins
    reg = *GPFSEL1;
    reg &= ~((7<<12)|(7<<15));  /* address of gpio 14, 15 */
    reg |=   (2<<12)|(2<<15);   /* set to alt5 */

    *GPFSEL1 = reg;            

    // ensures that GPIO pins are not affected by internal resistors
    *GPPUD = 0;                 /* Disable pull-up/down */
    reg=150;
    while ( reg-- )
    { 
        asm volatile("nop"); 
    }
    
    *GPPUDCLK0 = (1<<14)|(1<<15);

    //Stabilizing the Signal to ensure Proper Configuration
    reg=150; 
    while ( reg-- )
    {
        asm volatile("nop");
    }
    
    // ensures that GPIO pins are returned to a neutral state
    *GPPUDCLK0 = 0;             /* flush GPIO setup */

    *AUX_MU_CNTL = 3;           // Enable the transmitter and receiver.
    *AUX_MU_IER  = 3;           // Enable the interrupt.
}

/**
 * Send a character
 */
void uart_send(unsigned int c)
{
    /* Wait until UART transmitter is ready to accept new data. */
    do {

        asm volatile("nop");

    } while( ! ( *AUX_MU_LSR&0x20 ));

    /* write the character to the buffer */
    *AUX_MU_IO = c;

    // ensure proper line termination
    if ( c == '\n' )
    {
        do {

            asm volatile("nop");

        } while( ! ( *AUX_MU_LSR&0x20 ));

        //*AUX_MU_IO = '\r';
    }
}


/**
 * Display a string
 */
void uart_puts(char *s)
{
    while( *s )
    {

        uart_send(*s++);

    }
}

/**
 * Display a binary value in hexadecimal
 */

char uart_recv ( void )
{
	while(! ( *AUX_MU_LSR&0x01 )  ){
        asm volatile("nop");
    }
	return(*AUX_MU_IO&0xFF);
}

void itos(uint32_t value, char *str) {
    int i = 0;
    do {
        str[i++] = value % 10 + '0';
        value /= 10;
    } while (value != 0);
    str[i] = '\0';

    // Reverse the string
    int len = i;
    for (int j = 0; j < len / 2; j++) {
        char temp = str[j];
        str[j] = str[len - j - 1];
        str[len - j - 1] = temp;
    }
}