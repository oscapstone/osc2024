#include "command.h"

#include "initramfs.h"
#include "irq.h"
#include "mem.h"
#include "scheduler.h"
#include "shell.h"
#include "start.h"
#include "str.h"
#include "syscalls.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"
#include "virtm.h"

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

void cmd_hello() { uart_log(INFO, "Hello there!\n"); }

static void cmd_mem() {
  for (int i = BUDDY_MAX_ORDER; i >= 0; i--) {
    printFreeListByOrder(i);
  }
}

static void cmd_bd() {
  uart_puts("Buddy System: request order (0-");
  uart_dec(BUDDY_MAX_ORDER);
  uart_puts("): ");
  char *buf = kmalloc(SHELL_BUF_SIZE, SILENT);
  read_user_input(buf);
  int order = atoi(buf);
  if (buf[0] < '0' || buf[0] > '9' || order < 0 || order > BUDDY_MAX_ORDER) {
    uart_log(WARN, "Invalid order.\n");
  } else {
    demo_mem_rec *dmr = kmalloc(sizeof(demo_mem_rec), SILENT);
    dmr->ptr = kmalloc(PAGE_SIZE << order, SILENT);
    dmr->next = dmr_list;
    dmr_list = dmr;
  }
  kfree(buf, SILENT);
}

static void cmd_fm() {
  if (dmr_list == 0) {
    uart_log(WARN, "No memory block allocated.\n");
  } else {
    uart_log(INFO, "Freeing all allocated memory blocks...\n");
    do {
      demo_mem_rec *dmr = dmr_list;
      kfree(dmr->ptr, SILENT);
      dmr_list = dmr->next;
      kfree(dmr, SILENT);
    } while (dmr_list != 0);
  }
}

static void cmd_ca() {
  uart_log(INFO, "Dynamic Memory Allocator: request byte(s) (1-");
  uart_dec(PAGE_SIZE / 2);
  uart_puts("): ");
  char *buf = kmalloc(SHELL_BUF_SIZE, SILENT);
  read_user_input(buf);
  int size = atoi(buf);
  if (buf[0] == '\0' || size <= 0) {
    uart_log(WARN, "Invalid size.\n");
  } else {
    if (size > PAGE_SIZE / 2) {
      uart_log(
          WARN,
          "Size too large for dynamic allocator. Switching to Buddy System.\n");
    }
    demo_mem_rec *dmr = kmalloc(sizeof(demo_mem_rec), SILENT);
    dmr->ptr = kmalloc(size, SILENT);
    dmr->next = dmr_list;
    dmr_list = dmr;
  }
  kfree(buf, SILENT);
}

static void cmd_lab7() {
  char *buf = "/initramfs/vfs1.img";
  uart_log(INFO, "run: ");
  uart_puts(buf);
  uart_putc(NEWLINE);
  strcat(buf, "\0");
  vnode *target_file;
  int ret = vfs_lookup(buf, &target_file);  // assign the vnode to target
  if (ret != 0) {
    uart_log(WARN, "File not found.\n");
    return;
  }
  initramfs_run(target_file);
}

static void cmd_timer() {
  uart_puts("Duration(sec.): ");
  char *buf = kmalloc(SHELL_BUF_SIZE, SILENT);
  read_user_input(buf);
  int sec = atoi(buf);

  char *msg = kmalloc(SHELL_BUF_SIZE, SILENT);
  uart_puts(": ");
  read_user_input(msg);
  set_timer(msg, sec);
  kfree(buf, SILENT);
}

static void cmd_run() {
  // Get filename from user input
  uart_puts(": ");
  char *buf = kmalloc(SHELL_BUF_SIZE, SILENT);
  read_user_input(buf);
  vnode *target_file;
  int ret = vfs_lookup(buf, &target_file);  // assign the vnode to target
  kfree(buf, SILENT);
  if (ret != 0) {
    uart_log(WARN, "File not found.\n");
    return;
  }
  initramfs_run(target_file);
}

static void cmd_ls() {
  uart_log(INFO, "Listing initramfs files...\n");
  vnode *dirnode;
  if (vfs_lookup("/initramfs", &dirnode)) {
    uart_log(WARN, "initramfs not found.\n");
    return;
  }
  initramfs_inode *dir_inode = dirnode->internal;
  uart_puts(dir_inode->name);
  uart_putc(NEWLINE);
  vnode *ptr = dir_inode->entry;
  while (ptr) {
    initramfs_inode *inode = ptr->internal;
    uart_puts(inode->name);
    for (int i = 0; i < 20 - strlen(inode->name); i++) uart_putc(' ');
    uart_dec(inode->datasize);
    uart_putc(NEWLINE);
    ptr = ptr->next;
  }
}

static void cmd_reboot() {
  uart_log(INFO, "Start Rebooting...\n");
  // Reboot after 0x20000 ticks
  *PM_RSTC = PM_PASSWORD | 0x20;     // Full reset
  *PM_WDOG = PM_PASSWORD | 0x20000;  // Number of watchdog ticks
}

static void cmd_cancel() {
  uart_log(INFO, "Rebooting Attempt Aborted, if any.\n");
  *PM_RSTC = PM_PASSWORD | 0;
  *PM_WDOG = PM_PASSWORD | 0;
}

static void cmd_cat() {
  // Get filename from user input
  uart_puts(": ");
  char *buf = kmalloc(SHELL_BUF_SIZE, SILENT);
  read_user_input(buf);
  initramfs_cat(buf);
  kfree(buf, SILENT);
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
    {.name = "ls", .help = "List the content of initramfs", .func = cmd_ls},
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
    // retired
    {.name = "lab7", .help = "Showcase lab7 requirements", .func = cmd_lab7},
    {.name = END_OF_COMMAND_LIST}};
