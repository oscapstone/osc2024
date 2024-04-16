#include "mini_uart.h"
#include <stdint.h>


void main(){
  
  uart_init();
  
  uint32_t kernel_size = 0;

  for(int i = 0; i < 4; i++){
    kernel_size = (kernel_size << 8) | (uart_recv());
  }

  uart_sends("The kernel size: ");
  uart_sendh(kernel_size);
  uart_sendc('\n');

  char *kernel = (char*)0x80000;
  while(kernel_size--)
    *kernel++ = uart_recv();

  uart_sends("Finish transmission\n ");

  asm volatile (
    "mov x0, x5;"
    "mov x30, #0x80000;" // x30 used as the procedure link reg
    "ret"
  );

}