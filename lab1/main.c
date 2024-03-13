#include "bsp/mailbox.h"
#include "bsp/reboot.h"
#include "bsp/uart.h"
#include "kernel/io.h"
#include "lib/string.h"

void readcmd(char *x) {

  char input_char;
  x[0] = 0;
  int input_index = 0;
  // uart_puts("readcmd start\n");
  while ((input_char = read_c()) != '\n') {
    x[input_index] = input_char;
    input_index += 1;
    print_char(input_char);
  }

  // print_char('\n');
  x[input_index] = 0;
}

void shell() {

  print_string("\nYJack0000@Rpi3B+ >>> ");

  char command[256];
  readcmd(command);

  if (strcmp(command, "help") == 0) {
    print_string("\nhelp       : print this help menu\n");
    print_string("hello      : print Hello World!\n");
    print_string("mailbox    : print Hardware Information\n");
    print_string("reboot     : reboot the device");
  } else if (strcmp(command, "hello") == 0) {
    print_string("\nHello World!");
  } else if (strcmp(command, "mailbox") == 0) {
    print_string("\nMailbox info :\n");
    unsigned int r = get_board_revision();
    print_string("board revision : ");
    print_h(r);
    print_string("\r\n");
    unsigned int base_addr, size;
    get_memory_info(&base_addr, &size);
    print_string("ARM memory base address : ");
    print_h(base_addr);
    print_string("\r\n");
    print_string("ARM memory size : ");
    print_h(size);
    print_string("\r\n");
  } else if (strcmp(command, "reboot") == 0) {
    print_string("\nRebooting ...\n");
    reset(200);
  } else {
    print_string("\nCommand not found");
  }
}

int main() {
  // set up serial console
  uart_init();
  uart_puts("\nWelcome to AnonymousELF's shell");
  while (1) {
    shell();
  }

  return 0;
}
