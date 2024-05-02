#include "utils.h"
#include "helper.h"
#include "mmio.h"

#include "DEFINE.h"

#include <stdarg.h>
#include <stdint.h>


#define SIGN 1

char write_buffer[1024];
int write_tail = 0, write_ind = 0;
char recv_buffer[1024];
int recv_tail = 0, recv_ind = 0;

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

void uart_recv_string( char* buf ) {
	int m = 0;
	while(1) {
		while(1) {
			if(get32(AUX_MU_LSR_REG)&0x01) 
				break;
		}
		buf[m ++] = get32(AUX_MU_IO_REG) & 0xFF;
		if (buf[m - 1] == '\r') {
			m --;
			break;
		}
		if (buf[m - 1] == 127) {
			m --;
			continue;
		}
		uart_send(buf[m - 1]);
	}
	buf[m] = '\0';
}

uint32_t uart_recv_uint(void)
{
    char buf[4];

    for (int i = 0; i < 4; ++i) {
        buf[i] = uart_recv();
    }

    return *((uint32_t*)buf);
}

void uart_send_string(char* str) {
	for(int i = 0; str[i] != '\0'; i ++) {
		uart_send(str[i]);
	}
}


void uart_send_num(int64_t num, int base, int type)
{
    const char digits[16] = "0123456789ABCDEF";
    char tmp[66];
    int i;

    if (type & SIGN) {
        if (num < 0) {
            uart_send('-');
        }
    }

    i = 0;

    if (num == 0) {
        tmp[i++] = '0';
    } else {
        while (num != 0) {
            uint8_t r = ((uint64_t)num) % base;
            num = ((uint64_t)num) / base;
            tmp[i++] = digits[r];
        }
    }

    while (--i >= 0) {
        uart_send(tmp[i]);
    }
}

// Ref: https://elixir.bootlin.com/linux/v3.5/source/arch/x86/boot/printf.c#L115
void uart_printf(char *fmt, ...)
{
    const char *s;
    char c;
    uint64_t num;
    char width;

    va_list args;
    va_start(args, fmt);

    for (; *fmt; ++fmt) {
        if (*fmt != '%') {
            uart_send(*fmt);
            continue;
        }

        ++fmt;

        // Get width
        width = 0;
        if (fmt[0] == 'l' && fmt[1] == 'l') {
            width = 1;
            fmt += 2;
        }

        switch (*fmt) {
        case 'c':
            c = va_arg(args, uint32_t) & 0xff;
            uart_send(c);
            continue;
        case 'd':
            if (width) {
                num = va_arg(args, int64_t);
            } else {
                num = va_arg(args, int32_t);
            }
            uart_send_num(num, 10, SIGN);
            continue;
        case 's':
            s = va_arg(args, char *);
            uart_send_string((char*)s);
            continue;
        case 'x':
            if (width) {
                num = va_arg(args, uint64_t);
            } else {
                num = va_arg(args, uint32_t);
            }
            uart_send_num(num, 16, 0);
            continue;
        }
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



	put32(AUX_ENABLE,1);                   //Enable mini uart (this also enables access to its registers)
	put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
	put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
	put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
	put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
	put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200

	put32(AUX_MU_CNTL_REG,3);               //Finally, enable transmitter and receiver
	
	*UART_ICR  = 0x7FF; /* clear interrupts */
    *UART_FBRD = 0xB;
    *UART_LCRH = 0b11 << 5; /* 8n1 */
    *UART_CR   = 0x301;     /* enable Tx, Rx, FIFO */
    *UART_IMSC = 3 << 4; /* Tx, Rx */
}

void irq(int d) {
	if (d) {
		asm volatile ("msr DAIFclr, 0xf");
	}
	else {
		asm volatile ("msr DAIFset, 0xf");
	}
}

void change_read_irq(int d) {
	if (d) {
		mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | (0x1));
	}
	else {
		mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG & ~(0x1));
	}
}

void change_write_irq(int d) {
	if (d) {
		mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | (0x2));
	}
	else {
		mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG & ~(0x2));
	}
}

void uart_irq_on(){
    change_read_irq(1);
	*Enable_IRQs_1      |=   (1<<29);
}

void uart_irq_off(){
    change_read_irq(0);
	*Disable_IRQs_1     |=   (1<<29);
}

void uart_irq_send(char* str) {
	for (int i = 0; str[i] != '\0'; i ++) {
		write_buffer[write_tail ++] = str[i];
	}
	change_write_irq(1);
}

void uart_irq_read(char* str) {
	int ind = 0;
	while (recv_ind < recv_tail) {
		str[ind ++] = recv_buffer[recv_ind ++];
	}
}

void write_handler(){
	irq(0);	

    while(write_ind != write_tail){
		char c = write_buffer[write_ind++];
		*AUX_MU_IO_REG = (unsigned int)c;
	}

	irq(1);	
}

void recv_handler(){

	irq(0);	

	char c = (char)(*AUX_MU_IO_REG);
	if (c != 0) {
	    recv_buffer[recv_tail ++] = c;
	}
	change_read_irq(1);

	irq(1);	
}
