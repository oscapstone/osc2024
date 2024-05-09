#include "shell.h"
#include "uart.h"

int main(void) {
  uart_init();
  shell_run();

  return 0;
}