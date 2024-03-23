#ifndef __UART_H__
#define __UART_H__

#include "aux.h"
#include "gpio.h"
#include "mmio.h"

void uart_init();
void uart_write(char c);
void uart_read(char* buf, uint32_t size);
void uart_flush();
void uart_write_string(char* str);
void uart_puth(uint32_t d);
void uart_putc(char* buf, uint32_t size);
void delay(uint32_t t) {
    for (uint32_t i = 0; i < t; i++)
        asm volatile("nop");
}

#endif