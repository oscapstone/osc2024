#include "boot.h"

void uart_init()
{
	register unsigned int r = *GPFSEL1;
	r &= ~((7 << 12) | (7 << 15));
	r |= (2 << 12) | (2 << 15);
	*GPFSEL1 = r;

	*GPPUD = 0;
	for (int i = 0; i < 150; i++)
		asm volatile("nop");
	*GPPUDCLK0 = (1 << 14) | (1 << 15);
	for (int i = 0; i < 150; i++)
		asm volatile("nop");
	*GPPUD = 0;
	*GPPUDCLK0 = 0;

	*AUX_ENABLE |= 1;
	*AUX_MU_CNTL = 0;
	*AUX_MU_IER = 0;
	*AUX_MU_LCR = 3;
	*AUX_MU_MCR = 0;
	*AUX_MU_BAUD = 270;
	*AUX_MU_IIR = 6;
	*AUX_MU_CNTL = 3;
}

char uart_recv()
{
	while (!(*AUX_MU_LSR & 0x01))
		asm volatile("nop");
	return (char)(*AUX_MU_IO);
}

void uart_putc(char c)
{
	if (c == '\n')
		uart_putc('\r');
	while (!(*AUX_MU_LSR & 0x20))
		asm volatile("nop");
	*AUX_MU_IO = c;
}

void uart_puts(const char *s)
{
	while (*s)
		uart_putc(*s++);
}

int atoi(const char *s)
{
	int result = 0;
	int sign = 1;
	int i = 0;

	while (s[i] == ' ')
		i++;

	if (s[i] == '-') {
		sign = -1;
		i++;
	} else if (s[i] == '+')
		i++;

	while (s[i] >= '0' && s[i] <= '9') {
		result = result * 10 + (s[i] - '0');
		i++;
	}

	return sign * result;
}