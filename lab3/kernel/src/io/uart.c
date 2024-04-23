
#include "uart.h"
#include "utils/utils.h"

void uart_init() {

    // register: compile r into register instead of memory
    register unsigned int r;

    r = *GPFSEL1;
    r &= ~((7<<12)|(7<<15)); // gpio14, gpio15 clear to 0
	r |= (2<<12)|(2<<15);    // set gpio14 and 15 to 010/010 which is alt5
	*GPFSEL1 = r;          // from here activate Trasmitter&Receiver
    
	// according to BCM2835 ARM peripherals pg. 101 to setup the GPIO pull-up/down clock registers
	// disable pull-up and pull-down
    *GPPUD = 0;
	utils_delay(150);
	*GPPUDCLK0 = (1 << 14)|(1 << 15);
	utils_delay(150);
	*GPPUDCLK0 = 0;

	// need to delay before using uart
	utils_delay(500);

    // 1. set AUXENB register to enable mini UART.
	*AUX_ENABLE |= 1;
	// 2. Set AUX_MU_CNTL_REG to 0. Disable transmitter and receiver during configuration.
	*AUX_MU_CNTL = 0;
	// 3. Set AUX_MU_IER_REG to 0. Disable interrupt because currently you don’t need interrupt.
	*AUX_MU_IER = 0;
	// 4. Set AUX_MU_LCR_REG to 3. Set the data size to 8 bit.
	*AUX_MU_LCR = 3;
	// 5. Set AUX_MU_MCR_REG to 0. Don’t need auto flow control.
	*AUX_MU_MCR = 0;
	// 6. Set AUX_MU_BAUD to 270. Set baud rate to 115200
	// by calculation the BAUD reg should be 270.2673611111 = 270
	*AUX_MU_BAUD = 270;
	// 7. Set AUX_MU_IIR_REG to 6. No FIFO.
	// 31:8 Reserved, 7:6 FIFO enables, 5:4 zero, 2:1 READ bits WRITE bits
	//	    76543210
	//  0x6 00000110
	*AUX_MU_IIR = 0xc6;
	// 8. Set AUX_MU_CNTL_REG to 3. Enable the transmitter and receiver.
	*AUX_MU_CNTL = 3;

	// need to delay before using uart
	utils_delay(500);
}

void uart_send_char(unsigned int c) 
{
    // wait until we can send
    do 
    {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x20));
    *AUX_MU_IO = c;
}

char uart_get_char() 
{
    char r;
    do
    {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x01));

    r = (char) *AUX_MU_IO;
    return r;
}

void uart_send_string(const char* s) 
{
    while (*s) {
		// make new line
		if (*s == '\n')
			uart_send_char('\r');
        uart_send_char(*s++);
    }
}

void uart_send_nstring(unsigned int length, const char* s) 
{
    for (unsigned int i = 0; i < length; ++i) {
        uart_send_char(*s++);
    }
}


