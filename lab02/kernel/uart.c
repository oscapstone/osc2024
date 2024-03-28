#include "uart.h"
#include "utils.h"

#define AUX_ENABLE  ((volatile unsigned int *)(0x3F215004))
#define AUX_MU_IO   ((volatile unsigned int *)(0x3F215040))
#define AUX_MU_IER  ((volatile unsigned int *)(0x3F215044))
#define AUX_MU_IIR  ((volatile unsigned int *)(0x3F215048))
#define AUX_MU_LCR  ((volatile unsigned int *)(0x3F21504C))
#define AUX_MU_MCR  ((volatile unsigned int *)(0x3F215050))
#define AUX_MU_LSR  ((volatile unsigned int *)(0x3F215054))
#define AUX_MU_CNTL ((volatile unsigned int *)(0x3F215060))
#define AUX_MU_BAUD ((volatile unsigned int *)(0x3F215068))

#define GPFSEL1 ((volatile unsigned int *)(0x3F200004))
#define GPPUD ((volatile unsigned int *)(0x3F200094))
#define GPPUDCLK0 ((volatile unsigned int *)(0x3F200098))

void mini_uart_init() {
  register volatile unsigned int rsel = *GPFSEL1;
  register volatile unsigned int rclk0;
  
  rsel &= 0xFFFC0FFF;
  rsel |= ((2<<12)|(2<<15));
  *GPFSEL1 = rsel;
  *GPPUD = 0;
  
  int waitcycles = 150;
  while (waitcycles--)  asm volatile("nop");
  rclk0 = ((1<<14)|(1<<15));
  *GPPUDCLK0 = rclk0;
  waitcycles = 150;
  while (waitcycles--) asm volatile("nop");
    
  *AUX_ENABLE |= 1;
  *AUX_MU_CNTL = 0;
  *AUX_MU_IER = 0;
  *AUX_MU_LCR = 3;
  *AUX_MU_MCR = 0;
  *AUX_MU_BAUD = 270;
  *AUX_MU_IIR = 6;
  *AUX_MU_CNTL = 3;

}

void mini_uart_send(char c) {
  while (1) {
    if ((*AUX_MU_LSR)&0x20) break;
  }
  *AUX_MU_IO = c;
}

char mini_uart_recv() {
  while (1) {
    if ((*AUX_MU_LSR)&0x01) break;
  }
  return (*AUX_MU_IO)&0xFF;
}




void mini_uart_gets(char *buffer, int size) {
    char *p = buffer;

    for (int i = 0; i < size - 1; i++) {
        char c = mini_uart_recv();
        if (c == '\r' || c == '\n') {
            print_string("\r\n"); break;
        } else if (c < 31 || c > 128) {
            continue;
        }
        print_char(c); *p++ = c;
    }

    *p = '\0';
}
