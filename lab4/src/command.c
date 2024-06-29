#include "command.h"

#include "initramfs.h"
#include "mem.h"
#include "shell.h"
#include "string.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"

#define DEMO_ALLOCATED_LIST_MAX 64

static void *allocatedList[DEMO_ALLOCATED_LIST_MAX];
static int allocatedListSize = 0;

struct command commands[] = {
    {.name = "help", .help = "Display this help menu", .func = cmd_help},
    {.name = "hello", .help = "Print 'Hello there!'", .func = cmd_hello},
    {.name = "clear", .help = "Clear the screen", .func = cmd_clear},
    {.name = "reboot", .help = "Reboot the device", .func = cmd_reboot},
    {.name = "cancel", .help = "Cancel a scheduled reboot", .func = cmd_cancel},
    {.name = "info", .help = "Show hardware information", .func = cmd_info},
    {.name = "ls", .help = "List files in ramdisk", .func = initramfs_ls},
    {.name = "cat", .help = "Display content of ramdisk file", .func = cmd_cat},
    {.name = "run", .help = "Run a specified program", .func = cmd_run},
    {.name = "timer", .help = "Set timer with duration", .func = cmd_timer},
    {.name = "mem", .help = "Display free memory blocks", .func = cmd_mem},
    {.name = "bd", .help = "Allocate a memory block", .func = cmd_bd},
    {.name = "ca", .help = "Use the dynamic memory allocator", .func = cmd_ca},
    {.name = "fm", .help = "Free all memory in the demo list", .func = cmd_fm},
    {.name = "lab", .help = "Showcase lab requirements", .func = cmd_lab},
    {.name = END_OF_COMMAND_LIST}};

void cmd_mem() {
  for (int i = BUDDY_MAX_ORDER; i >= 0; i--) {
    printFreeListByOrder(i);
  }
}

static void allocatedListFullMessage() {
  uart_puts(
      "Max memory block allocation for demo is reached.\n"
      "Consider using 'fm' to clear all allocated memory blocks.\n");
}

void cmd_bd() {
  if (allocatedListSize >= DEMO_ALLOCATED_LIST_MAX) {
    allocatedListFullMessage();
    return;
  }
  uart_puts("Buddy System: request order (0-");
  uart_dec(BUDDY_MAX_ORDER);
  uart_puts("): ");
  char *buf = (char *)kmalloc(SHELL_BUF_SIZE, 1);
  read_user_input(buf);
  int order = atoi(buf);
  if (buf[0] < '0' || buf[0] > '9' || order < 0 || order > BUDDY_MAX_ORDER) {
    uart_puts("Invalid order.");
    uart_putc(NEWLINE);
  } else
    allocatedList[allocatedListSize++] = kmalloc(PAGE_SIZE << order, 0);
  kfree(buf, 1);
}

void cmd_fm() {
  if (allocatedListSize == 0) {
    uart_puts("No memory block allocated.");
    uart_putc(NEWLINE);
  } else {
    uart_puts("Freeing all allocated memory blocks...");
    uart_putc(NEWLINE);
    for (int i = 0; i < allocatedListSize; i++) {
      kfree(allocatedList[i], 0);
    }
    allocatedListSize = 0;
  }
}

void cmd_ca() {
  if (allocatedListSize >= DEMO_ALLOCATED_LIST_MAX) {
    allocatedListFullMessage();
    return;
  }
  uart_puts("Dynamic Memory Allocator: request byte(s) (1-");
  uart_dec(PAGE_SIZE / 2);
  uart_puts("): ");
  char *buf = (char *)kmalloc(SHELL_BUF_SIZE, 1);
  read_user_input(buf);
  int size = atoi(buf);
  if (buf[0] == '\0' || size <= 0) {
    uart_puts("Invalid size for dynamic allocator.");
    uart_putc(NEWLINE);
  } else {
    if (size > PAGE_SIZE / 2) {
      uart_puts(
          "[INFO] Size too large for dynamic allocator.\n"
          "[INFO] Switching to Buddy System.\n");
    }
    void *p = kmalloc(size, 0);
    allocatedList[allocatedListSize++] = p;
    // kfree(p, 0);
  }
  kfree(buf, 1);
}

static int *timeup = 0;
static void set_timeup(int *timeup) { *timeup = 1; }

void cmd_lab() {
  uart_puts("(1) Lab 3: UART async write");
  uart_putc(NEWLINE);
  uart_puts("(2) Lab 3: UART async read");
  uart_putc(NEWLINE);
  uart_puts("Please select: ");
  char *buf = (char *)kmalloc(SHELL_BUF_SIZE, 1);
  read_user_input(buf);
  switch (atoi(buf)) {
    case 1:
      uart_puts(": ");
      strncpy(buf, "[INFO] Async write: ", strlen("[INFO] Async write: "));
      read_user_input(buf + strlen("[INFO] Async write: "));
      // read_user_input(buf);
      uart_async_write(buf);
      break;
    case 2:
      uart_puts("(Please type something in 3 sec.)");
      uart_putc(NEWLINE);
      // set a timer to 3 sec.
      *timeup = 0;
      timer_add((void (*)(void *))set_timeup, (void *)timeup, 3);
      enable_uart_rx_interrupt();
      while (!*timeup);
      // time's up, get the buf content and print
      uart_async_read(buf, SHELL_BUF_SIZE);
      uart_puts("[INFO] Async read received: ");
      uart_puts(buf);
      uart_putc(NEWLINE);
      break;
    default:
      uart_puts("Option not found.");
      uart_putc(NEWLINE);
  }
  kfree(buf, 1);
}

void cmd_timer() {
  uart_puts("Duration(sec.): ");
  char *buf = (char *)kmalloc(SHELL_BUF_SIZE, 1);
  read_user_input(buf);
  int sec = atoi(buf);

  char *msg = (char *)kmalloc(SHELL_BUF_SIZE, 0);
  uart_puts(": ");
  read_user_input(msg);
  set_timer(msg, sec);
  kfree(buf, 1);
}

void cmd_run() {
  // Get filename from user input
  uart_puts(": ");
  char *buf = (char *)kmalloc(SHELL_BUF_SIZE, 1);
  read_user_input(buf);
  initramfs_run(buf);
  kfree(buf, 1);
}

void cmd_help() {
  uart_puts("-----------------------------------------");
  uart_putc(NEWLINE);
  struct command *cmd = commands;
  while (1) {
    if (!strcmp(cmd->name, END_OF_COMMAND_LIST)) {
      break;
    }
    uart_puts(cmd->name);
    uart_putc(TAB);
    uart_puts(": ");
    uart_puts(cmd->help);
    uart_putc(NEWLINE);
    cmd++;
  }
  uart_puts("-----------------------------------------");
  uart_putc(NEWLINE);
}

void cmd_hello() {
  uart_puts("Hello there!");
  uart_putc(NEWLINE);
}

void cmd_reboot() {
  uart_puts("Start Rebooting...");
  uart_putc(NEWLINE);
  // Reboot after 0x20000 ticks
  *PM_RSTC = PM_PASSWORD | 0x20;     // Full reset
  *PM_WDOG = PM_PASSWORD | 0x20000;  // Number of watchdog ticks
}

void cmd_cancel() {
  uart_puts("Rebooting Attempt Aborted, if any.");
  uart_putc(NEWLINE);
  *PM_RSTC = PM_PASSWORD | 0;
  *PM_WDOG = PM_PASSWORD | 0;
}

void cmd_info() {
  uart_puts("[INFO] board revision: ");
  uart_hex(BOARD_REVISION);
  uart_putc(NEWLINE);
  uart_puts("[INFO] device base memory address: ");
  uart_hex(BASE_MEMORY);
  uart_putc(NEWLINE);
  uart_puts("[INFO] device memory size: ");
  uart_hex(NUM_PAGES * PAGE_SIZE);
  uart_putc(NEWLINE);
}

void cmd_cat() {
  // Get filename from user input
  uart_puts(": ");
  char *buf = (char *)kmalloc(SHELL_BUF_SIZE, 1);
  read_user_input(buf);
  initramfs_cat(buf);
  kfree(buf, 1);
}

void cmd_clear() { uart_puts("\033[2J\033[H"); }
