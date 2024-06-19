#include "utils.h"
#include "peripherals/mini_uart_peripherals.h"
#include "peripherals/gpio.h"
#include "mini_uart.h"
#include "exception.h"
#include "tasklist.h"
#include "peripherals/irq.h"

#define BUFFER_SIZE 1024

char uart_read_buffer[BUFFER_SIZE];
char uart_write_buffer[BUFFER_SIZE];
int uart_read_index = 0;
int uart_read_head = 0;
int uart_write_index = 0;
int uart_write_head = 0;

void uart_init ( void )
{   
    /* setting gpio 14 and 15 */
	unsigned int selector;
    // MMIO_BASE = 0x3F000000 (phisical addr)
	selector = get32(GPFSEL1);              // get data from GPFSEL1(32 bits, in utils.S)
	selector &= ~(7 << 12);                 // clean gpio14 -> original & ~ 111b<<12 (12-14 bits)
	selector |= 2 << 12;                    // set alt5 (010b) for gpio14
	selector &= ~(7 << 15);                 // clean gpio15 -> original & ~ 111b<<15 (15-17 bits)
	selector |= 2 << 15;                    // set alt5 (010b) for gpio15
	put32(GPFSEL1, selector);               // put data back to GPFSEL1(32 bits, in utils.S)

    /* setting pull up/down register to disable GPIO pull up/down */
    /* If you use a particular pin as input and don't connect anything to this pin, 
    you will not be able to identify whether the value of the pin is 1 or 0. 
    In fact, the device will report random values. 
    The pull-up/pull-down mechanism allows you to overcome this issue. 
    If you set the pin to the pull-up state and nothing is connected to it, 
    it will report 1 all the time (for the pull-down state, the value will always be 0). 
    In our case, we need neither the pull-up nor the pull-down state, 
    because both the 14 and 15 pins are going to be connected all the time. 
    The pin state is preserved even after a reboot, so before using any pin, 
    we always have to initialize its state. 
    There are three available states: pull-up, pull-down, and neither (to remove the current pull-up or pull-down state), 
    and we need the third one.*/
	put32(GPPUD, 0);                        // delete current pull state
	delay(150);                             // Wait 150 cycles – this provides the required set-up time for the control signal
	put32(GPPUDCLK0, (1 << 14) | (1 << 15));// Write to GPPUDCLK0 to clock the control signal into the GPIO 14/15 – NOTE only the pads which receive a clock will be modified, all others will retain their previous state.
	delay(150); 
	put32(GPPUDCLK0, 0);                    // Write to GPPUDCLK0 to remove the clock

	put32(AUX_ENABLES, 1);                  // Enable mini uart (this also enables access to its registers)
	put32(AUX_MU_CNTL_REG, 0);              // Disable transmitter and receiver during configuration.
	put32(AUX_MU_IER_REG, 0);               // Disable receive and transmit interrupts
	put32(AUX_MU_LCR_REG, 3);               // Set the data size to 8 bit.
	put32(AUX_MU_MCR_REG, 0);               // Set RTS line to be always high -> Don’t need auto flow control.
	put32(AUX_MU_BAUD_REG, 270);            // Set baud rate to 115200 = (250[system clock] * 10^6) / (8 * (AUX_MU_BAUD_REG + 1))
                                            // “115200 baud” means that the serial port is capable of transferring a maximum of 115200 bits per second. 
    put32(AUX_MU_IIR_REG, 6);               // FIFO enabled (if bit 5 of LCR is set) and FIFO is empty
	put32(AUX_MU_CNTL_REG, 3);              // Finally, enable transmitter and receiver
}

char uart_recv ( void )                     // read data
{
	while(1) {
		if(get32(AUX_MU_LSR_REG) & 0x01)    // Check AUX_MU_LSR_REG’s data ready field. ready field is at bit 0, if value is 1 -> ready
			break;
	}
	return(get32(AUX_MU_IO_REG) & 0xFF);    // if set, read from AUX_MU_IO_REG
}

void uart_send ( char c )                   // write data
{
	while(1) {
		if(get32(AUX_MU_LSR_REG) & 0x20)    // Check AUX_MU_LSR_REG’s Transmitter empty field. empty field is at bit 5(100000b = 0x20), if value is 1 -> transmitter is empty
			break;
	}
	put32(AUX_MU_IO_REG, c);                // if set, write to AUX_MU_IO_REG
}

void uart_send_string(char* str)
{
	for (int i = 0; str[i] != '\0'; i ++) {
		uart_send((char)str[i]);
	}
}

void uart_send_string_int2hex(unsigned long value)
{	
	unsigned int n;
    for (int c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (value >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_send(n);
    }
}

void uart_enable_interrupt(void) 
{
	unsigned int aux_mu_ier = get32(AUX_MU_IER_REG); // Mini UART Interrupt Enable: control which UART event can trigger an interrupt
	aux_mu_ier |= 0x01;	// enable receiver interrupt
	// aux_mu_ier |= 0x02;	// enable sender interrupt
	put32(AUX_MU_IER_REG, aux_mu_ier);
	// ENABLE_IRQS_1: part of a broader set of interrupt controller registers which manage the enabling, disabling, and handling of various hardware interrupts.
	// `ENABLE_IRQS_1` is used to enable interrupts from various peripherals
	unsigned int enable_irqs_1 = get32(ENABLE_IRQS_1);
	enable_irqs_1 |= 0x01 << 29;
	put32(ENABLE_IRQS_1, enable_irqs_1);
}

void uart_disable_interrupt(void)
{	
	// do not trigger any interrupt when receiving data or sending data
	unsigned int aux_mu_ier = get32(AUX_MU_IER_REG);
	aux_mu_ier &= ~0x01;
	aux_mu_ier &= ~0x02;
	put32(AUX_MU_IER_REG, aux_mu_ier);

	unsigned int enable_irqs_1 = get32(ENABLE_IRQS_1);
	enable_irqs_1 &= ~(0x01 << 29);
	put32(ENABLE_IRQS_1, enable_irqs_1);
}

void uart_irq_handler(void) 
{
	unsigned int aux_mu_iir = get32(AUX_MU_IIR_REG); // Mini Uart Interrupt Identify
	unsigned int aux_mu_ier = get32(AUX_MU_IER_REG);
	// do not trigger any interrupt when receiving data or sending data
	aux_mu_ier &= ~0x02;
	aux_mu_ier &= ~0x01;
	put32(AUX_MU_IER_REG, aux_mu_ier);

	// check if receiver holds valid data
	if ((aux_mu_iir & 0x06) == 0x04) 
	{	
		// uart_send_string("uart_irq_handler -> irq_uart_rx_exception\r\n");
		create_task(irq_uart_rx_exception, 2);
		execute_tasks_preemptive();
	} 

	// check if transmitter is prepared to send data
	if ((aux_mu_iir & 0x06) == 0x02) 
	{	
		// uart_send_string("uart_irq_handler -> irq_uart_tx_exception\r\n");
		create_task(irq_uart_tx_exception, 1);
		execute_tasks_preemptive();
	}
}

void irq_uart_rx_exception(void) 
{
	if((uart_read_index + 1) % BUFFER_SIZE == uart_read_head) {
		// buffer is full, discard the data
		unsigned int aux_mu_ier = get32(AUX_MU_IER_REG);
		// only enable receiver interrupt for now
		aux_mu_ier &= ~0x01;
		put32(AUX_MU_IER_REG, aux_mu_ier);
		return;
	}
	char c = get32(AUX_MU_IO_REG)&0xFF;
	uart_read_buffer[uart_read_index++] = c;
	if (uart_read_index >= BUFFER_SIZE) {
		uart_read_index = 0;
	}
}

void irq_uart_tx_exception(void) 
{
	if (uart_write_index != uart_write_head) {
		put32(AUX_MU_IO_REG, uart_write_buffer[uart_write_index++]);
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
		unsigned int aux_mu_ier = get32(AUX_MU_IER_REG);
		aux_mu_ier |= 0x02;
		put32(AUX_MU_IER_REG, aux_mu_ier);
	} else {
		// no more data to send, disable transmit interrupt
		unsigned int aux_mu_ier = get32(AUX_MU_IER_REG);
		aux_mu_ier &= ~0x02;
		put32(AUX_MU_IER_REG, aux_mu_ier);
	}
}

char uart_async_recv(void) 
{
	while (uart_read_index == uart_read_head) {
		unsigned int aux_mu_ier = get32(AUX_MU_IER_REG);
		// only enable receiver interrupt for now
		aux_mu_ier |= 0x01;
		put32(AUX_MU_IER_REG, aux_mu_ier);
	}

	disable_interrupt();
	char c = uart_read_buffer[uart_read_head++];
	if (uart_read_head >= BUFFER_SIZE) {
		uart_read_head = 0;
	}
	enable_interrupt();
	return c;
}

void uart_async_send_string(const char* str)
{
	// if the write buffer is full, wait for it to be empty
	while ((uart_write_index + 1) % BUFFER_SIZE == uart_write_head) {
		unsigned int aux_mu_ier = get32(AUX_MU_IER_REG);
		// only enable transmit interrupt for now
		aux_mu_ier |= 0x02;
		put32(AUX_MU_IER_REG, aux_mu_ier);
	}
	disable_interrupt();
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
	enable_interrupt();
	// enable transmit interrupt
	unsigned int aux_mu_ier = get32(AUX_MU_IER_REG);
	aux_mu_ier |= 0x02;
	put32(AUX_MU_IER_REG, aux_mu_ier);
	return;
}

