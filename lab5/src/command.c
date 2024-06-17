#include "command.h"

#include "initramfs.h"
#include "mem.h"
#include "scheduler.h"
#include "shell.h"
#include "str.h"
#include "syscalls.h"
#include "test.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"

typedef struct {
  void *ptr;
  void *next;
} demo_mem_rec;

static demo_mem_rec *dmr_list = 0;

void cmd_info() {
  uart_log(INFO, "board revision: ");
  uart_hex(BOARD_REVISION);
  uart_putc(NEWLINE);
  uart_log(INFO, "device base memory address: ");
  uart_hex(BASE_MEMORY);
  uart_putc(NEWLINE);
  uart_log(INFO, "device memory size: ");
  uart_hex(NUM_PAGES * PAGE_SIZE);
  uart_putc(NEWLINE);
}

void cmd_hello() {
  uart_puts("Hello there!");
  uart_putc(NEWLINE);
}

static void cmd_mem() {
  for (int i = BUDDY_MAX_ORDER; i >= 0; i--) {
    printFreeListByOrder(i);
  }
}

static void cmd_bd() {
  uart_puts("Buddy System: request order (0-");
  uart_dec(BUDDY_MAX_ORDER);
  uart_puts("): ");
  char *buf = kmalloc(SHELL_BUF_SIZE, 1);
  read_user_input(buf);
  int order = atoi(buf);
  if (buf[0] < '0' || buf[0] > '9' || order < 0 || order > BUDDY_MAX_ORDER) {
    uart_puts("Invalid order.");
    uart_putc(NEWLINE);
  } else {
    demo_mem_rec *dmr = kmalloc(sizeof(demo_mem_rec), 1);
    dmr->ptr = kmalloc(PAGE_SIZE << order, 0);
    dmr->next = dmr_list;
    dmr_list = dmr;
  }
  kfree(buf, 1);
}

static void cmd_fm() {
  if (dmr_list == 0) {
    uart_puts("No memory block allocated.");
    uart_putc(NEWLINE);
  } else {
    uart_puts("Freeing all allocated memory blocks...");
    uart_putc(NEWLINE);
    do {
      demo_mem_rec *dmr = dmr_list;
      kfree(dmr->ptr, 0);
      dmr_list = dmr->next;
      kfree(dmr, 1);
    } while (dmr_list != 0);
  }
}

static void cmd_ca() {
  uart_log(INFO, "Dynamic Memory Allocator: request byte(s) (1-");
  uart_dec(PAGE_SIZE / 2);
  uart_puts("): ");
  char *buf = kmalloc(SHELL_BUF_SIZE, 1);
  read_user_input(buf);
  int size = atoi(buf);
  if (buf[0] == '\0' || size <= 0) {
    uart_log(WARN, "Invalid size.\n");
  } else {
    if (size > PAGE_SIZE / 2) {
      uart_log(
          INFO,
          "Size too large for dynamic allocator. Switching to Buddy System.\n");
    }
    demo_mem_rec *dmr = kmalloc(sizeof(demo_mem_rec), 1);
    dmr->ptr = kmalloc(size, 0);
    dmr->next = dmr_list;
    dmr_list = dmr;
  }
  kfree(buf, 1);
}

static void cmd_lab() {
  uart_puts(
      "(1) Lab 3: UART async write\n"
      "(2) Lab 3: UART async read\n"
      "(3) Lab 5: Thread creation test\n"
      "(4) Lab 5: Fork test\n"
      ": ");
  char *buf = kmalloc(SHELL_BUF_SIZE, 1);
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
      int *timeup = 0;
      timer_add((void (*)(void *))set_timeup, (void *)timeup, 3);
      enable_uart_rx_interrupt();
      while (!*timeup);
      // time's up, get the buf content and print
      uart_async_read(buf, SHELL_BUF_SIZE);
      uart_puts("[INFO] Async read received: ");
      uart_puts(buf);
      uart_putc(NEWLINE);
      break;
    case 3:
      for (int i = 0; i < 3; i++) kcreate_thread(thread_test);
      idle();
      break;
    case 4:
      kcreate_thread(run_fork_test);
      idle();
      break;
    default:
      uart_puts("Option not found.");
      uart_putc(NEWLINE);
  }
  kfree(buf, 1);
}

static void cmd_timer() {
  uart_puts("Duration(sec.): ");
  char *buf = kmalloc(SHELL_BUF_SIZE, 1);
  read_user_input(buf);
  int sec = atoi(buf);

  char *msg = kmalloc(SHELL_BUF_SIZE, 0);
  uart_puts(": ");
  read_user_input(msg);
  set_timer(msg, sec);
  kfree(buf, 1);
}

static void cmd_run() {
  // Get filename from user input
  uart_puts(": ");
  char *buf = kmalloc(SHELL_BUF_SIZE, 1);
  read_user_input(buf);
  initramfs_run(buf);
  kfree(buf, 1);
}

static void cmd_reboot() {
  uart_puts("Start Rebooting...");
  uart_putc(NEWLINE);
  // Reboot after 0x20000 ticks
  *PM_RSTC = PM_PASSWORD | 0x20;     // Full reset
  *PM_WDOG = PM_PASSWORD | 0x20000;  // Number of watchdog ticks
}

static void cmd_cancel() {
  uart_puts("Rebooting Attempt Aborted, if any.");
  uart_putc(NEWLINE);
  *PM_RSTC = PM_PASSWORD | 0;
  *PM_WDOG = PM_PASSWORD | 0;
}

static void cmd_cat() {
  // Get filename from user input
  uart_puts(": ");
  char *buf = kmalloc(SHELL_BUF_SIZE, 1);
  read_user_input(buf);
  initramfs_cat(buf);
  kfree(buf, 1);
}

static void cmd_clear() { uart_clear(); }

static void cmd_help() {
  uart_puts("-----------------------------------------");
  uart_putc(NEWLINE);
  struct command *cmd = cmd_list;
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

struct command cmd_list[] = {
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
    // lab4
    {.name = "mem", .help = "Display free memory blocks", .func = cmd_mem},
    {.name = "bd", .help = "Allocate a memory block", .func = cmd_bd},
    {.name = "ca", .help = "Use the dynamic memory allocator", .func = cmd_ca},
    // lab5
    {.name = "fm", .help = "Free all memory in the demo list", .func = cmd_fm},
    {.name = "pid", .help = "List all running threads", .func = list_tcircle},
    {.name = "lab", .help = "Showcase lab requirements", .func = cmd_lab},
    {.name = END_OF_COMMAND_LIST}};
