#include "command.h"

#include "initramfs.h"
#include "mbox.h"
#include "shell.h"
#include "string.h"
#include "uart.h"

struct command commands[] = {
    {.name = "help", .help = "print this help menu", .func = cmd_help},
    {.name = "hello", .help = "print Hello World!", .func = cmd_hello},
    {.name = "clear", .help = "clear screen", .func = cmd_clear},
    {.name = "reboot", .help = "reboot the device", .func = cmd_reboot},
    {.name = "cancel", .help = "cancel reboot", .func = cmd_cancel},
    {.name = "info", .help = "hardware information", .func = cmd_info},
    {.name = "ls", .help = "list ramdisk files", .func = cmd_ls},
    {.name = "cat", .help = "print ramdisk file", .func = cmd_cat},
    {.name = "NULL"}  // Must put a NULL command at the end!
};

void cmd_help() {
  int i = 0;

  uart_puts("------------------------------\n");
  while (1) {
    if (!strcmp(commands[i].name, "NULL")) {
      break;
    }
    uart_puts(commands[i].name);
    uart_puts("\t: ");
    uart_puts(commands[i].help);
    uart_putc('\n');
    i++;
  }
  uart_puts("------------------------------\n\n");
}

void cmd_hello() { uart_puts("Hello there!\n\n"); }

void cmd_reboot() {
  uart_puts("Start Rebooting...\n\n");
  // Reboot after 0x20000 ticks
  *PM_RSTC = PM_PASSWORD | 0x20;     // Full reset
  *PM_WDOG = PM_PASSWORD | 0x20000;  // Number of watchdog ticks
}

void cmd_cancel() {
  uart_puts("Rebooting Attempt Aborted, if any.\n\n");
  *PM_RSTC = PM_PASSWORD | 0;
  *PM_WDOG = PM_PASSWORD | 0;
}

void cmd_info() {
  // Get board revision
  get_board_revision();
  uart_puts("board revision: ");
  uart_hex(mbox[5]);
  uart_putc('\n');

  // Get ARM memory base address and size
  get_arm_memory_status();
  uart_puts("device base memory address: ");
  uart_hex(mbox[5]);
  uart_putc('\n');
  uart_puts("device memory size: ");
  uart_hex(mbox[6]);
  uart_puts("\n\n");
}

void cmd_ls() {
  initramfs_list();
  uart_putc('\n');
}

void cmd_cat() {
  // Get filename from user input
  uart_puts(": ");
  char buf[MAX_BUF_SIZE + 1];
  int idx = 0;
  while (1) {
    char c = uart_getc();
    uart_putc(c);
    if (c == '\n') {
      buf[idx] = '\0';
      break;
    } else if (c == 127) {  // Backspaces
      if (idx > 0) {
        buf[idx--] = 0;
        uart_puts("\b \b");
        // uart_putc('\b');
        // uart_putc(' ');
        // uart_putc('\b');
      }
    } else {
      buf[idx++] = c;
    }
  }
  initramfs_cat(buf);
  uart_putc('\n');
}

void cmd_clear() { uart_puts("\033[2J\033[H"); }