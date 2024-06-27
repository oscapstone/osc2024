#include "gpio.h"
#include "mini_uart.h"
#include "interrupt.h"
#include "string.h"
#include "timer.h"

char uart_rx_buffer[BUFFER_SIZE] = {0};
char uart_tx_buffer[BUFFER_SIZE] = {0};
int uart_rx_max = 0;
int uart_rx_index = 0;
int uart_tx_max = 0;
int uart_tx_index = 0;

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

void uart_sendl(unsigned long long h){
  unsigned int n;
  int c;
  for (c = 60; c >= 0; c -= 4) {
    n = (h >> c) & 0xf;
    n += n > 9 ? 0x37 : '0';
    uart_send(n);
  }
  return;
}

void uart_sendi(int num) {
    char buffer[12]; // Buffer to hold the string representation of the integer
    int i = 0;
    int isNegative = 0;

    // Handle negative numbers
    if (num < 0) {
        isNegative = 1;
        num = -num;
    }

    do {
        buffer[i++] = num % 10 + '0';
        num /= 10;
    } while (num);

    if (isNegative)
        buffer[i++] = '-';

    buffer[i] = '\0';

    int j = 0;
    char temp;
    for (j = 0; j < i / 2; j++) {
        temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    uart_sends(buffer);
}

char uart_async_recv(){
  *AUX_MU_IER_REG &= ~(0x1);
  // while(uart_rx_index==uart_rx_max);
    // *AUX_MU_IER_REG |= (0x1);
  char c;
  // int i = 0;
  if(uart_rx_index!=uart_rx_max){
    c = uart_rx_buffer[uart_rx_index];
    uart_rx_index = (uart_rx_index+1)%BUFFER_SIZE;
  }
  else { return 0;}
  return c;
}

void uart_async_gets(char *s, int len){
  *AUX_MU_IER_REG &= ~(0x1);
  int i = 0;
  while(uart_rx_index!=uart_rx_max){
    s[i] = uart_rx_buffer[uart_rx_index];
    uart_rx_index = (uart_rx_index+1)%BUFFER_SIZE;
    i++;
  }
  // for(i = 0;i < uart_rx_max && i < len; i++){
  //   s[i] = uart_rx_buffer[i];
  //   if(s[i] == '\n')
  //     break;
  // }
  s[i] = '\0';
  // uart_rx_max = 0;
}

void uart_async_send(const char c){
  if(c == '\n'){
    uart_tx_buffer[uart_tx_max] = '\r';
    uart_tx_max = (uart_tx_max + 1)%BUFFER_SIZE;
  }
  uart_tx_buffer[uart_tx_max] = c;
  uart_tx_max = (uart_tx_max + 1)%BUFFER_SIZE;

  *AUX_MU_IER_REG |= (0x2);
}

void uart_async_sends(const char *s){
  // *AUX_MU_IER_REG &= ~(0x2);
  int len = strlen(s);

  for(int i = 0; i < len; i++){
    if(s[i] == '\n'){
      uart_tx_buffer[uart_tx_max] = '\r';
      uart_tx_max = (uart_tx_max + 1)%BUFFER_SIZE;
    }
    uart_tx_buffer[uart_tx_max] = s[i];
    uart_tx_max = (uart_tx_max + 1)%BUFFER_SIZE;
  }
  // uart_tx_buffer[uart_tx_max] = '\0';
  *AUX_MU_IER_REG |= (0x2);
}

void uart_async_test(){
  char opt[256];
  uart_sends("test for (1)async (2)preemption > ");
  gets(opt);
  switch(atoi(opt)){
    case 1:
      uart_sends("uart async test!\n");
      mini_uart_irq_enable();
      *AUX_MU_IER_REG |= (0x1);

      // delay(1500000000); // for qemu
      delay(10000000); // for raspi3b+
      char user_input[BUFFER_SIZE];

      // int i = 0;
      // while(1){
      //   user_input[i] = uart_async_recv();
      //   if(i > uart_rx_max||user_input[i] == '\n')
      //     break;
      //   i++;
      // }
      // user_input[i] = '\0';
      uart_async_gets(user_input,BUFFER_SIZE);
      uart_async_sends("asynchornous receive: ");
      uart_async_sends(user_input);
      uart_async_send('\n');
      delay(10000000);

      mini_uart_irq_disable();
      break;
    case 2:
      /* Need to open irq_test() in exec_task() in interrupt.c */
      uart_sends("preemption test!\n");
      mini_uart_irq_enable();
      uart_async_sends("test\n");
      delay(10000000);
      mini_uart_irq_disable();
      break;
    default:
      uart_sends("Undefined inupt.\n");
      break;
  }
}
