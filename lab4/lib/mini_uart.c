#include "utils.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"
#include "exception.h"

void uart_send ( char c )
{
	while (!(get32(AUX_MU_LSR_REG) & 0x20)) { 
		asm volatile("nop"); 
	}
	put32(AUX_MU_IO_REG, c);
}

char uart_recv ( void )
{
	while(1) {
		if(get32(AUX_MU_LSR_REG)&0x01) 
			break;
	}
	return(get32(AUX_MU_IO_REG)&0xFF);
}

void uart_send_string(char* str)
{
	for (int i = 0; str[i] != '\0'; i ++) {
		if(str[i] == '\n') { uart_send('\r'); }
		uart_send((char)str[i]);
	}
}

void uart_send_hex(unsigned int d) {
    unsigned int c;

    for (int i = 28; i >= 0; i -= 4) {
        // Highest 4 bits
        c = (d >> i) & 0xF;
        c = (c > 9) ? (0x37 + c) : (0x30 + c); // 0x37 + 10 = 0x41 ('A'), 0x30 = '0'
        uart_send(c);
    }
}

void uart_send_uint(unsigned int d) {
	unsigned int base = 100000000;
	unsigned int digit;
	int lead_zero = 1;
	char ch;

	while(base > 0) {
		digit = d / base;
		if(digit > 0) {
			lead_zero = 0;
			d -= digit * base;
		}
		if(lead_zero == 0) {
			ch = '0' + digit;
			uart_send(ch);
		}
		base /= 10;
	}
	if(lead_zero == 1) { uart_send('0'); }
}

void uart_init ( void )
{
	unsigned int selector;

	selector = get32(GPFSEL1);
	selector &= ~(7<<12);                   // clean gpio14
	selector |= 2<<12;                      // set alt5 for gpio14
	selector &= ~(7<<15);                   // clean gpio15
	selector |= 2<<15;                      // set alt5 for gpio15
	put32(GPFSEL1,selector);

	put32(GPPUD,0);
	delay(150);
	put32(GPPUDCLK0,(1<<14)|(1<<15));
	delay(150);
	put32(GPPUDCLK0,0);

	put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to its registers)
	put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
	put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
	//put32(AUX_MU_IER_REG, 1);
	put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
	put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
	put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200
	put32(AUX_MU_IIR_REG, 6);			//Interrupt identify no fifo
	put32(AUX_MU_CNTL_REG,3);               //Finally, enable transmitter and receiver

	//put32(ENABLE_IRQS_1, (1<<29));
}


/* asynchronous read and write */
char read_buffer[MAX_BUFFER_SIZE];
char write_buffer[MAX_BUFFER_SIZE];
int r_head, r_tail;
int w_head, w_tail;

void async_uart_init() {
	r_head = 0;
	r_tail = 0;
	w_head = 0;
	w_tail = 0;
}

void enable_uart_tx_interrupt() {
	volatile unsigned int *ptr = (unsigned int*)AUX_MU_IER_REG;
	*ptr = *ptr | 0x2; // set bits[1]
}

void disable_uart_tx_interrupt() {
	volatile unsigned int *ptr = (unsigned int*)AUX_MU_IER_REG;
	*ptr = *ptr & ~(0x2); // reset bits[1]
}

void enable_uart_rx_interrupt() {
	volatile unsigned int *ptr = (unsigned int*)AUX_MU_IER_REG;
	*ptr = *ptr | 0x1; // set bits[0]
}

void disable_uart_rx_interrupt() {
	volatile unsigned int *ptr = (unsigned int*) AUX_MU_IER_REG;
	*ptr = *ptr & ~(0x1); // reset bits[0]
}

void async_uart_send(char c) {
	write_buffer[w_head] = c;
	if(w_head == MAX_BUFFER_SIZE-1) { w_head = 0; }
	else { w_head++; }
	enable_uart_tx_interrupt();
}

void async_uart_send_string(char* str) {
	for (int i = 0; str[i] != '\0'; i ++) {
		if(str[i] == '\n') {
			write_buffer[w_head] = '\r';
			if(w_head == MAX_BUFFER_SIZE-1) { w_head = 0; }
			else { w_head++; }
		}
		write_buffer[w_head] = str[i];
		if(w_head == MAX_BUFFER_SIZE-1) { w_head = 0; }
		else { w_head++; }
	}
	enable_uart_tx_interrupt();
}

char async_uart_recv(void) {
	char c;
	
	while(r_head == r_tail) { asm volatile("nop"); }
	c = read_buffer[r_tail];
	if(r_tail == MAX_BUFFER_SIZE-1) { r_tail = 0; }
	else { r_tail++; }

	return c;
}

void uart_irq_handler() {
    unsigned int is_rx, is_tx;
	/**
	 * AUX_MU_IIR_REG[2:1]
	 * 00: No Interrupts
	 * 01: Transmit holding register empty
	 * 10: Receiver holds valid byte
	 * 11: <Not possible>
	 */
    is_rx = get32(AUX_MU_IIR_REG) & 0x4;
    is_tx = get32(AUX_MU_IIR_REG) & 0x2;

    //uart_send_string("UART IRQ handler\r\n");

	/**
	 * AUX_MU_LSR_REG[0]
	 * This bit is set if the receive FIFO holds at least 1 symbol.
	 */
    if(is_rx) {
        while(get32(AUX_MU_LSR_REG) & 0x1) {
            read_buffer[r_head] = uart_recv();
            if(r_head == MAX_BUFFER_SIZE-1) { r_head = 0; }
            else { r_head++; }
        }
    }

    if(is_tx) {
        while(w_tail != w_head) {
            uart_send(write_buffer[w_tail]);
            if(w_tail == MAX_BUFFER_SIZE-1) { w_tail = 0; }
            else { w_tail++; }
        }
		disable_uart_tx_interrupt();
		enable_uart_rx_interrupt();
    }
}

void test_async() {
	char buff[MAX_BUFFER_SIZE];
	int hidx = 0, tidx = 0;

	enable_aux_interrupt();
	enable_uart_rx_interrupt();
	for(int i = 0; i < 1000; i++) { asm volatile("nop"); }

	while(1) {
		while (1) {
			buff[hidx] = async_uart_recv();
			if (buff[hidx] == '\r') break;
			hidx++;
		}
		buff[hidx+1] = '\n';
		buff[hidx+2] = '\0';
		async_uart_send_string(buff+tidx);
		tidx = hidx;
	}	

	disable_aux_interrupt();
	while(1) { asm volatile("nop"); }
}