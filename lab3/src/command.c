#include "command.h"

#include "initramfs.h"
#include "malloc.h"
#include "mbox.h"
#include "shell.h"
#include "string.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"

struct command commands[] = {
    {.name = "help", .help = "print this help menu", .func = cmd_help},
    {.name = "hello", .help = "print Hello there!", .func = cmd_hello},
    {.name = "clear", .help = "clear screen", .func = cmd_clear},
    {.name = "reboot", .help = "reboot the device", .func = cmd_reboot},
    {.name = "cancel", .help = "cancel reboot", .func = cmd_cancel},
    {.name = "info", .help = "hardware information", .func = cmd_info},
    {.name = "ls", .help = "list ramdisk files", .func = cmd_ls},
    {.name = "cat", .help = "print ramdisk file", .func = cmd_cat},
    {.name = "run", .help = "run a program", .func = cmd_run},
    {.name = "timer", .help = "set timer", .func = cmd_timer},
    {.name = "lab", .help = "showcase lab requirements", .func = cmd_lab},
    {.name = END_OF_COMMAND_LIST}};

void cmd_lab() {
  char buf[SHELL_BUF_SIZE];
  uart_puts("(1) UART async write");
  uart_putc(NEWLINE);
  uart_puts("(2) UART async read");
  uart_putc(NEWLINE);
  uart_puts("Please select: ");
  read_user_input(buf);
  switch (atoi(buf)) {
    case 1:
      uart_puts(": ");
      read_user_input(buf);
      uart_async_write(buf);
      // uart_async_write("[INFO] Test the UART async write function\n");
      break;
    case 2:
      uart_puts("(Please type something in 3 sec.)");
      uart_putc(NEWLINE);
      // set a timer to 3 sec.
      reset_timeup();
      timer_add((void (*)(void *))set_timeup, (void *)0, 3);
      uart_enable_rx_interrupt();
      while (!get_timeup());
      // time's up, get the buf content and print
      uart_async_read(buf, 256);
      uart_puts(buf);
      uart_putc(NEWLINE);
      uart_putc(NEWLINE);
      break;
    default:
      uart_puts("Option not found.");
      uart_putc(NEWLINE);
      break;
  }
}

void cmd_timer() {
  char sec[SHELL_BUF_SIZE];
  uart_puts("Duration(sec.): ");
  read_user_input(sec);

  char *msg = simple_malloc(SHELL_BUF_SIZE);
  uart_puts(": ");
  read_user_input(msg);
  set_timer(msg, atoi(sec));
}

void cmd_run() {
  // Get filename from user input
  char buffer[SHELL_BUF_SIZE];
  uart_puts(": ");
  read_user_input(buffer);
  initramfs_run(buffer);
}

void cmd_help() {
  int i = 0;

  uart_puts("------------------------------");
  uart_putc(NEWLINE);
  while (1) {
    if (!strcmp(commands[i].name, END_OF_COMMAND_LIST)) {
      break;
    }
    uart_puts(commands[i].name);
    uart_putc(TAB);
    uart_puts(": ");
    uart_puts(commands[i].help);
    uart_putc(NEWLINE);
    i++;
  }
  uart_puts("------------------------------");
  uart_putc(NEWLINE);
  uart_putc(NEWLINE);
}

void cmd_hello() {
  uart_puts("Hello there!");
  uart_putc(NEWLINE);
  uart_putc(NEWLINE);
}

void cmd_reboot() {
  uart_puts("Start Rebooting...");
  uart_putc(NEWLINE);
  uart_putc(NEWLINE);

  // Reboot after 0x20000 ticks
  *PM_RSTC = PM_PASSWORD | 0x20;     // Full reset
  *PM_WDOG = PM_PASSWORD | 0x20000;  // Number of watchdog ticks
}

void cmd_cancel() {
  uart_puts("Rebooting Attempt Aborted, if any.");
  uart_putc(NEWLINE);
  uart_putc(NEWLINE);
  *PM_RSTC = PM_PASSWORD | 0;
  *PM_WDOG = PM_PASSWORD | 0;
}

void cmd_info() {
  // Get board revision
  get_board_revision();
  uart_puts("board revision: ");
  uart_hex(mbox[5]);
  uart_putc(NEWLINE);

  // Get ARM memory base address and size
  get_arm_memory_status();
  uart_puts("device base memory address: ");
  uart_hex(mbox[5]);
  uart_putc(NEWLINE);
  uart_puts("device memory size: ");
  uart_hex(mbox[6]);
  uart_putc(NEWLINE);
  uart_putc(NEWLINE);
}

void cmd_ls() {
  initramfs_list();
  uart_putc(NEWLINE);
}

void cmd_cat() {
  // Get filename from user input
  uart_puts(": ");
  char buf[MAX_BUF_SIZE + 1];
  int idx = 0;
  while (1) {
    char c = uart_getc();
    uart_putc(c);
    if (c == NEWLINE) {
      buf[idx] = '\0';
      break;
    } else if (c == 127) {  // Backspaces
      if (idx > 0) {
        buf[idx--] = 0;
        uart_putc(BACKSPACE);
        uart_putc(' ');
        uart_putc(BACKSPACE);
      }
    } else {
      buf[idx++] = c;
    }
  }
  initramfs_cat(buf);
  uart_putc(NEWLINE);
}

void cmd_clear() { uart_puts("\033[2J\033[H"); }
