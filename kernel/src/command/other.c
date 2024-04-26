#include "command/other.h"

#include "lib/uart.h"

void hello_cmd(char *args, size_t arg_size) { uart_println("Hello World!"); }
