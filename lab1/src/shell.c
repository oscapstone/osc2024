#include "shell.h"
#include "mbox.h"
#include "string.h"
#include "uart.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

void set(long addr, unsigned int value) {
  volatile unsigned int *point = (unsigned int *)addr;
  *point = value;
}

// reboot after watchdog timer expire
void reset(int tick) {
  uart_println("rebooting...");

  set(PM_RSTC, PM_PASSWORD | 0x20); // full reset
  set(PM_WDOG, PM_PASSWORD | tick); // number of watchdog tick
}

void cancel_reset() {
  set(PM_RSTC, PM_PASSWORD | 0); // cancel reset
  set(PM_WDOG, PM_PASSWORD | 0); // number of watchdog tick
}

void handle_line(char *line) {
  if (strncmp(line, "help", 4) == 0) {
    uart_println("help\t: print this help menu");
    uart_println("hello\t: print Hello World!");
    uart_println("board\t: show board info");
    uart_println("reboot\t: reboot the device");
    return;
  }

  if (strncmp(line, "hello", 5) == 0) {
    uart_println("Hello World!");
    return;
  }

  if (strncmp(line, "reboot", 6) == 0) {
    reset(100);
    return;
  }

  if (strncmp(line, "board", 5) == 0) {
    unsigned int board_revision;
    if (get_board_revision(&board_revision) == 0) {
      uart_print("Board revision number is: ");
      uart_hex(board_revision);
      uart_print("\n");
    } else {
      uart_println("Unable to get board revision!");
    }

    unsigned int base_addr, mem_size;
    if (get_arm_mem_info(&base_addr, &mem_size) == 0) {
      uart_print("Base addr: ");
      uart_hex(base_addr);
      uart_print("\n");

      uart_print("Mem size: ");
      uart_hex(mem_size);
      uart_print("\n");
    } else {
      uart_println("Fail to get mem inforamtion!");
    }
  }
}

void shell_loop() {
  char cmd[256] = {};
  int cmd_index = 0;
  while (1) {

    // prompt
    if (cmd_index == 0) {
      uart_print("$ ");
    }

    // echo user input
    char c = uart_read();
    uart_print(&c);

    cmd[cmd_index++] = c;

    // consume input until newline
    if (c != '\n') {
      continue;
    }

    cmd[cmd_index] = '\0';
    cmd_index = 0;

    handle_line(cmd);
  }
}
