#include "utils.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"
#include "peripherals/irq.h"


char read_buffer[MAX_BUF_LEN];
char write_buffer[MAX_BUF_LEN];
int read_index_cur = 0;
int read_index_tail = 0;
int write_index_cur = 0;
int write_index_tail = 0;


void uart_send ( char c )
{
	while(1) {
		if(get32(AUX_MU_LSR_REG)&0x20) 
			break;
	}
	put32(AUX_MU_IO_REG,c);
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
		uart_send((char)str[i]);
	}
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
	put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
	put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
	put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200
	put32(AUX_MU_IIR_REG,6);		//clear the receive FIFO and the transmit FIFO
	
	put32(AUX_MU_CNTL_REG,3);               //Finally, enable transmitter and receiver
}

int uart_irq_gets(char *buf){
    int buf_index = 0;
    char input_char;

    while(1){
        input_char = (char)uart_irq_getc();
        //uart_putc(input_char);
        if(input_char == -1)
            continue;
        // Get non ASCII code
        if(input_char > 127 || input_char < 0){
            //uart_puts("\nwarning: Get non ASCII code\n");
            continue;
        }
        if(buf_index < MAX_BUF_LEN)
            buf[buf_index++] = (input_char == '\r' ? '\n' : input_char);
        // should replace with parsed char
        //uart_putc(input_char);
        // when receving ENTER
        if(input_char == '\n'){
            // add EOF after '\n'
            buf[buf_index] = '\0';
            break;
        }
    }

    return buf_index;
}

void uart_irq_on(){
    
    put32(AUX_MU_IER_REG,get32(AUX_MU_IER_REG) | 1);
    //*AUX_MU_IER_REG     |=   1;  //enable receive interrupt(transmit will be handled in its function)
    //*Enable_IRQs_1      |=   (1<<29);
    put32(ENABLE_IRQS_1,get32(ENABLE_IRQS_1)|(1<<29));
}

void uart_irq_off(){
    //mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG & ~(0x1));  //disable receive interrupt
    put32(AUX_MU_IER_REG,get32(AUX_MU_IER_REG)& ~(0x1));
    //*Disable_IRQs_1     |=   (1<<29);
    put32(DISABLE_IRQS_1,get32(DISABLE_IRQS_1)|(1<<29));
}

int uart_irq_getc(){
    // there's char in buffer
    if(read_index_cur != read_index_tail){
        int c = (int)read_buffer[read_index_cur++];
        // make it circular
        read_index_cur = read_index_cur % MAX_BUF_LEN;

        return c;
    }
    else
        return -1;
}

void uart_irq_putc(unsigned char c){
    write_buffer[write_index_tail++] = c;
    write_index_tail = write_index_tail % MAX_BUF_LEN;
    //p.12 The AUX_MU_IER_REG register is primary used to enable interrupts 
    //mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | 0x2);
    put32(AUX_MU_IER_REG,get32(AUX_MU_IER_REG) | 0x2);
}

void uart_irq_puts(const char *str){
    int i;

    for(i = 0; str[i] != '\0'; i++){
        if(str[i] == '\n'){
            write_buffer[write_index_tail++] = '\r';
            write_index_tail = write_index_tail % MAX_BUF_LEN;
        }
        write_buffer[write_index_tail++] = str[i];
        write_index_tail = write_index_tail % MAX_BUF_LEN;
    }

    //mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | 0x2);
    put32(AUX_MU_IER_REG,get32(AUX_MU_IER_REG) | 0x2);
}