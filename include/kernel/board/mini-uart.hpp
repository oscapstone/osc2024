#pragma once

extern const bool& mini_uart_is_async;
extern int mini_uart_delay;

void mini_uart_use_async(bool use);

void mini_uart_enqueue();
void mini_uart_handler(void*);

char mini_uart_getc_raw_async();
void mini_uart_putc_raw_async(char c);

char mini_uart_getc_raw();
void mini_uart_putc_raw(char c);

void mini_uart_setup();

char mini_uart_getc_raw_sync();
void mini_uart_putc_raw_sync(char c);

// TODO: move to io
int mini_uart_getline_echo(char* buffer, int size);
