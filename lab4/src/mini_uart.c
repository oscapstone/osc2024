#include "mini_uart.h"
#include "c_utils.h"
#include "exception.h"
#include "peripherals/p_mini_uart.h"
#include "peripherals/gpio.h"
#include "peripherals/irq.h"
#include "tasklist.h"

char uart_read_buffer[BUFFER_SIZE];
char uart_write_buffer[BUFFER_SIZE];
int uart_read_index = 0;
int uart_read_head = 0;
int uart_write_index = 0;
int uart_write_head = 0;

void uart_send ( char c )
{
	if (c == '\n')
		uart_send('\r');
	while(1) {
		if(*AUX_MU_LSR_REG&0x20) 
			break;
	}
	*AUX_MU_IO_REG = c;
}

char uart_recv ( void )
{
	while(1) {
		if(*AUX_MU_LSR_REG&0x01) 
			break;
	}
	return(*AUX_MU_IO_REG);
}

void uart_send_string(const char* str)
{
	while (*str) {
		uart_send(*str++);
	}
}

void irq_uart_rx_exception() {
	if((uart_read_index + 1) % BUFFER_SIZE == uart_read_head) {
		// buffer is full, discard the data
		unsigned int ier = *AUX_MU_IER_REG;
		// only enable receiver interrupt for now
		ier &= ~0x01;
		*AUX_MU_IER_REG = ier;
		return;
	}
	char c = *AUX_MU_IO_REG&0xFF;
	uart_read_buffer[uart_read_index++] = c;
	if (uart_read_index >= BUFFER_SIZE) {
		uart_read_index = 0;
	}
}

void irq_uart_tx_exception() {
	if (uart_write_index != uart_write_head) {
		*AUX_MU_IO_REG = uart_write_buffer[uart_write_index++];
		// uart_send_string("before delayed\n");
		// for(unsigned int i=0;i<100000000;i++){
		// 	asm volatile("nop");
		// 	asm volatile("nop");
		// 	asm volatile("nop");
		// }
		// uart_send_string("after delayed\n");

		// uart_send_string("delayed finished\n");
		if (uart_write_index >= BUFFER_SIZE) {
			uart_write_index = 0;
		}
		unsigned int ier = *AUX_MU_IER_REG;
		ier |= 0x02;
		*AUX_MU_IER_REG = ier;
	} else {
		// no more data to send, disable transmit interrupt
		unsigned int ier = *AUX_MU_IER_REG;
		ier &= ~0x02;
		*AUX_MU_IER_REG = ier;
	}
}

void uart_irq_handler() {
	unsigned int iir = *AUX_MU_IIR_REG;
	unsigned int ier = *AUX_MU_IER_REG;
	ier &= ~0x02;
	ier &= ~0x01;
	*AUX_MU_IER_REG = ier;
	// check if it's a receive interrupt
	if ((iir & 0x06) == 0x04) {
		// uart_send_string("Receive interrupt\n");	
		create_task(irq_uart_rx_exception, 2);
		execute_tasks_preemptive();
	} 
	// check if it's a transmit interrupt
	if ((iir & 0x06) == 0x02) {
		// uart_send_string("Transmit interrupt\n");
		create_task(irq_uart_tx_exception, 1);
		execute_tasks_preemptive();
	}
}


char uart_async_recv( void ) {

	while (uart_read_index == uart_read_head) {
		unsigned int ier = *AUX_MU_IER_REG;
		// only enable receiver interrupt for now
		ier |= 0x01;
		*AUX_MU_IER_REG = ier;
	}

	el1_interrupt_disable();
	char c = uart_read_buffer[uart_read_head++];
	if (uart_read_head >= BUFFER_SIZE) {
		uart_read_head = 0;
	}
	el1_interrupt_enable();
	return c;
}

void uart_async_send_string(const char* str){

	// if the write buffer is full, wait for it to be empty
	while ((uart_write_index + 1) % BUFFER_SIZE == uart_write_head) {
		unsigned int ier = *AUX_MU_IER_REG;
		// only enable transmit interrupt for now
		ier |= 0x02;
		*AUX_MU_IER_REG = ier;
	}
	el1_interrupt_disable();
	while(*str){
		if(*str == '\n') {
			uart_write_buffer[uart_write_head++] = '\r';
			uart_write_buffer[uart_write_head++] = '\n';
			str++;
		} else {
			uart_write_buffer[uart_write_head++] = *str++;
		}
		if (uart_write_head >= BUFFER_SIZE) {
			uart_write_head = 0;
		}
	}
	el1_interrupt_enable();
	// enable transmit interrupt
	unsigned int ier = *AUX_MU_IER_REG;
	ier |= 0x02;
	*AUX_MU_IER_REG = ier;
	return;
}

void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_send(n);
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
	*AUX_MU_IER_REG = 0;                //Disable receive and transmit interrupts
	*AUX_MU_LCR_REG = 3;                //Enable 8 bit mode
	*AUX_MU_MCR_REG = 0;                //Set RTS line to be always high
	*AUX_MU_BAUD_REG = 270;             //Set baud rate to 115200
	*AUX_MU_CNTL_REG = 3;               //Finally, enable transmitter and receiver
}

void uart_enable_interrupt( void ) {
	unsigned int ier = *AUX_MU_IER_REG;
	// only enable receiver interrupt for now
	ier |= 0x01;
	// ier |= 0x02;
	*AUX_MU_IER_REG = ier;

	unsigned int enable_irqs_1 = *ENABLE_IRQS_1;
	enable_irqs_1 |= 0x01 << 29;
	*ENABLE_IRQS_1 = enable_irqs_1;
}

void uart_disable_interrupt( void ) {
	unsigned int ier = *AUX_MU_IER_REG;
	// only enable receiver interrupt for now
	ier &= ~0x01;
	ier &= ~0x02;
	*AUX_MU_IER_REG = ier;

	unsigned int enable_irqs_1 = *ENABLE_IRQS_1;
	enable_irqs_1 &= ~(0x01 << 29);
	*ENABLE_IRQS_1 = enable_irqs_1;
}
