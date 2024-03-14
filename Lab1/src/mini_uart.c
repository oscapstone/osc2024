#include "gpio.h"
#include "mini_uart.h"

void delay(unsigned int cycles)
{
  while (cycles--)
  {
    asm volatile("nop");
  }
}

void uart_init()
{
  unsigned int selector;

  selector = *GPFSEL1; //for 10-19
  selector &= ~(7 << 12); // reset gpio14(12~14) to 000
  selector |= (2 << 12);  // set gpio14 to alternate function 5(010) for tx
  selector &= ~(7 << 15); // reset 15(15~17) to 000
  selector |= (2 << 15);  // set gpio15 to alternate function 5(010) for rx
  *GPFSEL1 = selector;

  *GPPUD = 0; // disable pull up/down, because can't read the current state, so reset
  delay(150); // wait 150 cycles
  *GPPUDCLK0 |= (1 << 14) | (1 << 15); // 0 for 0-31
  delay(150);
  *GPPUD = 0;
  *GPPUDCLK0 = 0;

  *AUX_ENABLES = 1;       // enable mini-uart, bit 0 for mini_uart, if set to enable, uart would recieve data immediately
  *AUX_MU_CNTL_REG = 0;   // disable transmitter and receiver
  *AUX_MU_IER_REG = 0;    // disable interrupt
  *AUX_MU_LCR_REG = 3;    // set data size 8 bits
  *AUX_MU_MCR_REG = 0;    // disable auto flow control
  *AUX_MU_BAUD_REG = 270; // set baud rate to 115200, after booting the system is 250MHZ

  *AUX_MU_IIR_REG = 6;  // clear FIFO bit1 for recv; bit2 for transmit
  *AUX_MU_CNTL_REG = 3; // Enable transmitter and receiver; bit0: receiver; bit 1: transmitter
}

char uart_recv()
{
  char c;
  while (1)
  {
    if ((*AUX_MU_LSR_REG) & 0x01) // check if data ready
      break;
  }
  c = (char)*AUX_MU_IO_REG;

  return c == '\r' ? '\n' : c;
}

void uart_send(unsigned int c)
{
  while (1)
  {
    if ((*AUX_MU_LSR_REG) & 0x20) // bit6 for transmitter idle, check if transmit FIFO is empty
      break;
  }

  *AUX_MU_IO_REG = c;
}

void uart_sends(const char *s)
{
  while (*s)
  {
    if (*s == '\n')
    {
      uart_send('\r');
    }
    uart_send(*s++);
  }
}

void uart_sendc(const char c)
{
  if (c == '\n')
  {
    uart_send('\r');
  }
  uart_send(c);
}

void uart_sendh(unsigned int h) {
  unsigned int n;
  int c;
  for (c = 28; c >= 0; c -= 4) {
    n = (h >> c) & 0xf;
    n += n > 9 ? 0x37 : '0';
    uart_send(n);
  }
  return;
}