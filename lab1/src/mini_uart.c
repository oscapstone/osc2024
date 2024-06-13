#include "utils.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"

void uart_send ( char c )
{
	while(1) {
		if(get32(AUX_MU_LSR_REG)&0x20) //Bit five, if set to 1, tells us that the transmitter is empty, meaning that we can write to the UART
			break;
	}
	put32(AUX_MU_IO_REG,c);
}

char uart_recv ( void )
{
	while(1) {
		if(get32(AUX_MU_LSR_REG)&0x01) //Bit zero, if set to 1, indicates that the data is ready
			break;
	}
	return(get32(AUX_MU_IO_REG)&0xFF);
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

	selector = get32(GPFSEL1);				// control alternative functions for pins 10-19
	selector &= ~(7<<12);                   // clean gpio14
	selector |= 2<<12;                      // set alt5(010) for gpio14
	selector &= ~(7<<15);                   // clean gpio15
	selector |= 2<<15;                      // set alt5 for gpio15
	put32(GPFSEL1,selector);

	put32(GPPUD,0);							// remove both the pull-up and pull-down states from pins 14 and 15
	delay(150);
	put32(GPPUDCLK0,(1<<14)|(1<<15));		// clock the control signal into the GPIO pads
	delay(150);
	put32(GPPUDCLK0,0);						// remove the clock

	put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to its registers)
	put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
	put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
	put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
	put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
	put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200
	put32(AUX_MU_IIR_REG, 6);			//Interrupt identify no fifo
	put32(AUX_MU_CNTL_REG,3);               //Finally, enable transmitter and receiver
}
