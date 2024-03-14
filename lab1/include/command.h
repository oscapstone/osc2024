#ifndef COMMAND_H
#define COMMAND_H
#include "peripherals/mini_uart.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC     0x3F10001c
#define PM_WDOG     0x3F100024

void input_buffer_overflow_message(char[]);
void command_help();
void command_hello();
void command_not_found(char*);
void command_reboot();
void command_info();
void command_clear();

#endif