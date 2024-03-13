#include "../include/uart1.h"
#include "../include/bcm2837/rpi_gpio.h"
#include "../include/bcm2837/rpi_uart1.h"

void uart_init()
{
	register unsigned int r;

	/* initialize UART */
	*AUX_ENABLES |= 1;    // enable UART1
	*AUX_MU_CNTL_REG = 0; // disable TX/RX

	/* configure UART */
	*AUX_MU_IER_REG = 0;    // disable interrupt
	*AUX_MU_LCR_REG = 3;    // 8 bit data size
	*AUX_MU_MCR_REG = 0;    // disable flow control
	*AUX_MU_BAUD_REG = 270; // 115200 baud rate
	*AUX_MU_IIR_REG = 6;    // disable FIFO

	/* map UART1 to GPIO pins */
	r = *GPFSEL1;    // load GPFSEL1(register) to r
	r &= ~(7 << 12); // clean gpio14
	r |= 2 << 12;    // set gpio14 to alt5
	r &= ~(7 << 15); // clean gpio15
	r |= 2 << 15;    // set gpio15 to alt5
	*GPFSEL1 = r;

	/* enable pin 14, 15 - ref: Page 101 */
	*GPPUD = 0; // gpio pull-up/down, enable/disable gpio 上拉/下拉電阻
	r = 150;    // wait
	while (r--) {
		asm volatile("nop");
	}
	*GPPUDCLK0 = (1 << 14) | (1 << 15); // 啟用 pull-up/down 電阻
	r = 150;
	while (r--) {
		asm volatile("nop");
	}
	*GPPUDCLK0 = 0; // clear register

	*AUX_MU_CNTL_REG = 3; // Enable transmitter and receiver
}

char uart_recv()
{
	char r;
	while (!(*AUX_MU_LSR_REG & 0x01)) { }; // wait until LSR(fifo receiver) receive data
	r = (char)(*AUX_MU_IO_REG);            // save data in 'r'
	return r == '\r' ? '\n' : r;
}

void uart_send(unsigned int c)
{
	while (!(*AUX_MU_LSR_REG & 0x20)) { };
	*AUX_MU_IO_REG = c;
}

void uart_puts(char* str)
{
	while (*str) {
		if (*str == '\n')
			uart_send('\r');
		uart_send(*str++);
	}
}

void uart_2hex(unsigned int d)
{
	unsigned int n;
	int c;
	for (c = 28; c >= 0; c -= 4) {
		n = (d >> c) & 0xF;       // get 4 bits
		n += n > 9 ? 0x37 : 0x30; // if > 10, trans it to 'A'
		uart_send(n);
	}
}