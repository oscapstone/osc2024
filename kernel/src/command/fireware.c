#include "command/fireware.h"

#include "mbox.h"
#include "uart.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

void board_cmd(char *args, size_t arg_size) {
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

void set(long addr, unsigned int value) {
  volatile unsigned int *point = (unsigned int *)addr;
  *point = value;
}

// reboot after watchdog timer expire
void reset(int tick) {
  uart_println("rebooting...");

  set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
  set(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset() {
  set(PM_RSTC, PM_PASSWORD | 0);  // cancel reset
  set(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}

void reboot_cmd(char *args, size_t arg_size) { reset(100); }
