#include "command/other.h"

#include "uart.h"

void hello_cmd(char *args, u32_t arg_size) { uart_println("Hello World!"); }
