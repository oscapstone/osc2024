#ifndef _UART_H
#define _UART_H

#include "type.h"
#include "gpio.h"

void uart_init();
uint8_t uart_read();
uint8_t uart_read_bin();
void uart_write(char);

#endif