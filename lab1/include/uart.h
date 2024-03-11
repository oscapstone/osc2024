#ifndef _UART_H_
#define _UART_H_

void uart_init();               // initialize the device and maps it to the GPIO ports
void uart_send(unsigned int c); // send a character
char uart_getc();               // receive a character
void uart_puts(char *s);        // send a string
void uart_hex(unsigned int d);  // send a hex number

#endif