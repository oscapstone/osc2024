#include "mini_uart.h"
#include <stdint.h>


void main(){
  
  uart_init();
  
  // uint8_t size_bytes[4];
  uint32_t kernel_size = 0;

  uart_sends("\nWait for loading...\n");


  // while(1)
  //   uart_send('1');
  for(int i = 0; i < 4; i++){
    // size_bytes[i] = uart_recv();
    kernel_size = (kernel_size << 8) | (uart_recv() & 0xff);
    // char c = uart_recv();
    // uart_send(c);
  }
  // kernel_size = (size_bytes[0]<<24)|(size_bytes[1]<<16)|(size_bytes[2]<<8)|size_bytes[3];
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