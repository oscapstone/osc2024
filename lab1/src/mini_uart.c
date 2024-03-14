#include "utils.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"

void uart_send (char c)
{
	while(1) {
		if(get32(AUX_MU_LSR_REG)&0x20) 
			break;
	}
	put32(AUX_MU_IO_REG,c);
}

char uart_recv (void)
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
		uart_send((char)str[i]);
	}
}

void uart_2hex(unsigned int d)
{ 
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        n = (d>>c) & 0xF;
        n += n>9 ? 0x37: 0x30;
        uart_send(n);
    }
}

void uart_init (void)
{
	unsigned int selector;

	selector = get32(GPFSEL1);
	selector &= ~(7<<12);                   // clean gpio14
	selector |= 2<<12;                      // set alt5 for gpio14
	selector &= ~(7<<15);                   // clean gpio15
	selector |= 2<<15;                      // set alt5 for gpio15
	put32(GPFSEL1,selector);                // 將 selector 的值放到 GPFSEL1

	put32(GPPUD,0);                         // 寫入 GPPUD 來設置所需的控制信號（pull-up、pull-down或0）
	delay(150);
	put32(GPPUDCLK0,(1<<14)|(1<<15));       // 寫入GPPUDCLK0 對 pin14 跟 pin15 做修改
	delay(150);
	put32(GPPUDCLK0,0);                     // Write GPPUD 以移除控制信號

	put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to its registers)
	put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
	put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
	put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
	put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
	put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200

	put32(AUX_MU_CNTL_REG,3);               //Finally, enable transmitter and receiver
}
