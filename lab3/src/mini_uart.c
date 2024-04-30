#include "../include/utils.h"
#include "../include/peripherals/mini_uart.h"
#include "../include/peripherals/gpio.h"
#include "../include/mini_uart.h"
#include <stdint.h>

/* for async part*/
#define BUFFER_MAX_SIZE 0x100
#define BUFFER_END 0xFF

static uint8_t _read_buffer[BUFFER_MAX_SIZE];
static uint8_t _write_buffer[BUFFER_MAX_SIZE];
static uint32_t _read_start = 0, _read_end = 0;
static uint32_t _write_start = 0, _write_end = 0;

static void read_buffer_add(uint8_t c)
{
	_read_buffer[_read_end++] = c;
	_read_end &= BUFFER_END;
}

static uint8_t read_buffer_get()
{
	uint8_t c = _read_buffer[_read_start++];
	_read_start &= BUFFER_END;
	return c;
}

static void write_buffer_add(uint8_t c)
{
	_write_buffer[_write_end++] = c;
	_write_end &= BUFFER_END;
}

static uint8_t write_buffer_get()
{
	uint8_t c = _write_buffer[_write_start++];
	_write_start &= BUFFER_END;
	return c;
}

static uint32_t write_buffer_empty()
{
	return _write_start == _write_end;
}



/* for polling part */

void uart_send ( char c )
{
	while(1) {
		if(*AUX_MU_LSR_REG & 0x20) 
			break;
	}
	*AUX_MU_IO_REG = c;
}

char uart_recv ( void )
{
	while(1) {
		if(*AUX_MU_LSR_REG & 0x01) 
			break;
	}
	return(*AUX_MU_IO_REG & 0xFF);
}

void uart_send_string(char* str)
{
	for (int i = 0; str[i] != '\0'; i ++) {
		uart_send((char)str[i]);
	}
}

void uart_init ( void )
{
	unsigned int selector;

	selector = *GPFSEL1;
	selector &= ~(7<<12);                   // clean gpio14
	selector |= 2<<12;                      // set alt5 for gpio14
	selector &= ~(7<<15);                   // clean gpio15
	selector |= 2<<15;                      // set alt5 for gpio15
	*GPFSEL1 = selector;

	*GPPUD = 0;
	delay(150);
	*GPPUDCLK0 = (1<<14)|(1<<15);
	delay(150);
	*GPPUDCLK0 = 0;

	*AUX_ENABLES = 1;                   //Enable mini uart (this also enables access to its registers)
	*AUX_MU_CNTL_REG = 0;               //Disable auto flow control and disable receiver and transmitter (for now)
	*AUX_MU_IER_REG = 0;                //Enable receive
	*AUX_MU_LCR_REG = 3;                //Enable 8 bit mode
	*AUX_MU_MCR_REG = 0;                //Set RTS line to be always high
	*AUX_MU_BAUD_REG = 270;             //Set baud rate to 115200
	*AUX_MU_IIR_REG = 6;

	*AUX_MU_CNTL_REG = 3;               //Finally, enable transmitter and receiver
}

/**
 * Display a binary value in hexadecimal
 */
void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // get highest tetrad
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        uart_send(n);
    }
}


/* for aync part */
uint8_t uart_async_recv()
{
	set_rx_interrupts();
	while (_read_start == _read_end) {
		asm volatile ("nop");
	}

	// disable_irq_interrupts();
	uint8_t c = read_buffer_get();
	// enable_irq_interrupts();
	return c;
}

void uart_async_send(uint8_t c)
{
	write_buffer_add(c);
	set_tx_interrupts();
}

void uart_rx_handler()
{
	// disable_irq_interrupts();
	read_buffer_add(uart_recv());
	// enable_irq_interrupts();
	// clr_rx_interrupts();
}

void uart_tx_handler()
{
	// disable_irq_interrupts();
	while (!write_buffer_empty()) {
		delay(1 << 28);                // (1 << 28) for qemu
		uint8_t c = write_buffer_get();
		uart_send(c);
	}
	uart_send('\n');
	// clr_tx_interrupts();
}

void uart_async_demo()
{
	enable_irq_interrupts();
	uint8_t c = uart_async_recv();
	while (c != '\r') {
		write_buffer_add(c);
		c = uart_async_recv();
	}
	write_buffer_add(c);
	// clr_rx_interrupts();
	set_tx_interrupts();
}