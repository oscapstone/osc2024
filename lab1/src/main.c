#include "shell.h"
#include "uart.h"

void main() {
  // set up serial console
  uart_init();

  // say hello
  uart_println("Hello World from boot");

  shell_loop();
}
