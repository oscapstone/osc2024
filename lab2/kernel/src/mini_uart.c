#include "utils.h"
#include "helper.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"

#include <stdarg.h>
#include <stdint.h>


#define SIGN 1

void uart_send ( char c )
{
	while(1) {
		if(get32(AUX_MU_LSR_REG)&0x20) 
			break;
	}
	put32(AUX_MU_IO_REG,c);
}

void uart_send_string(char* str)
{
	for (int i = 0; str[i] != '\0'; i ++) {
		uart_send((char)str[i]);
	}
}

void output(char* str) {
	uart_send_string(str);
	uart_send_string("\r\n");
}

void output_hex(unsigned int x) {
	char buf[100];
	hex_to_string(x, buf);
	output(buf);
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

	put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to its registers)
	put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
	put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
	put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
	put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
	put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200

	put32(AUX_MU_CNTL_REG,3);               //Finally, enable transmitter and receiver
}
