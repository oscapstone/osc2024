#include "shell.h"
#include "bool.h"
#include "cpio.h"
#include "def.h"
#include "dtb.h"
#include "irq.h"
#include "logo.h"
#include "mailbox.h"
#include "memory.h"
#include "mini_uart.h"
#include "peripheral/pm.h"
#include "reboot.h"
#include "string.h"
#include "timer.h"
#include "utils.h"

#define BUFFER_SIZE 128

static char cmd[BUFFER_SIZE];

int read_command(char *cmd) {
  int i = 0;
  char c;

  while (i < BUFFER_SIZE) {
    c = uart_recv();
    uart_send(c);
    switch (c) {
    case '\n':
      uart_send('\r');
      cmd[i] = '\0';
      return 0;
    case '\b':
      if (i > 0)
        cmd[--i] = '\0';
      break;
    default:
      cmd[i++] = c;
      break;
    }
  }
  return 1;
}

void help(void) {
  uart_send_string("Shell for Raspberry Pi 3B+\n"
                   "Available commands:\n"
                   "  help        - display this information\n"
                   "  hello       - display hello world\n"
                   "  reboot      - reboot the system\n"
                   "  info        - display system information\n"
                   "  ls          - list files in the initramfs\n"
                   "  cat         - display file content\n"
                   "  malloc      - allocate memory\n"
                   "  print_dtb   - print device tree blob\n"
                   "  logo        - print raspberry pi logo\n"
                   "  exec        - execute a program\n"
                   "  async       - test uart async\n"
                   "  set_timeout - set core timer timeout\n");
}

void hello(void) { uart_send_string("Hello, world!\n"); }

void info(void) {
  print_board_revision();
  print_arm_memory();
}

void parse_command(char *cmd) {
  char *cmd_name = str_tok(cmd, " ");
  if (!str_cmp(cmd_name, "help"))
    help();
  else if (!str_cmp(cmd_name, "hello"))
    hello();
  else if (!str_cmp(cmd_name, "reboot"))
    reset(10000);
  else if (!str_cmp(cmd_name, "info"))
    info();
  else if (!str_cmp(cmd_name, "ls"))
    ls();
  else if (!str_cmp(cmd_name, "cat")) {
    char *file_name = str_tok(NULL, " ");
    if (!file_name) {
      uart_send_string("Usage: cat <filename>\n");
      return;
    }
    cat(file_name);
  } else if (!str_cmp(cmd_name, "malloc")) {
    char *size = str_tok(NULL, " ");
    if (!size) {
      uart_send_string("Usage: malloc <size>\n");
      return;
    }
    int dec_size = decstr2int(size);
    if (dec_size < 0) {
      uart_send_string("Invalid size\n");
      return;
    }
    void *ret = mem_alloc(decstr2int(size));
    if (!ret) {
      uart_send_string("Failed to allocate ");
      uart_send_string(size);
      uart_send_string(" bytes\n");
    } else {
      uart_send_string("Memory allocated at 0x");
      uart_send_hex((unsigned long)ret);
      uart_send_string("\n");
    }
  } else if (!str_cmp(cmd_name, "print_dtb"))
    fdt_traverse(print_dtb);
  else if (!str_cmp(cmd_name, "logo"))
    send_logo();
  else if (!str_cmp(cmd_name, "asia_godtone"))
    send_asiagodtone();
  else if (!str_cmp(cmd_name, "exec")) {
    char *file_name = str_tok(NULL, " ");
    if (!file_name) {
      uart_send_string("Usage: exec <filename>\n");
      return;
    }
    exec(file_name);
  } else if (!str_cmp(cmd_name, "async")) {
    test_uart_async();
  } else if (!str_cmp(cmd_name, "set_timeout")) {
    char *msg = str_tok(NULL, " ");
    char *duration = str_tok(NULL, " ");
    char *repeat = str_tok(NULL, " ");
    if (!msg || !duration || !repeat) {
      uart_send_string("Usage: set_timeout <message> <seconds> <repeat>\n");
      return;
    }
    int seconds = decstr2int(duration);
    if (seconds < 0) {
      uart_send_string("Invalid time\n");
      return;
    }

    bool periodic = (bool)decstr2int(repeat);
    add_timer(print_msg, msg, seconds, periodic);

  } else if (!str_cmp(cmd_name, "fuck")) {
    disable_core0_timer();
    uart_send_string("No more core timer interrupt!!\n");
  } else if (!str_cmp(cmd_name, "alloc_page")) {
    char *size = str_tok(NULL, " ");
    if (!size)
      uart_send_string("Usage: alloc_page <size>\n");
    int request = decstr2int(size);
    if (request < 0) {
      uart_send_string("Invalid size\n");
      return;
    }
    uint8_t *page_ptr = alloc_pages((size_t)request * PAGE_SIZE);
  } else if (!str_cmp(cmd_name, "test_page_alloc")) {
    test_page_alloc();

  } else if (!str_cmp(cmd_name, "print_freelist")) {
    print_free_list();
  } else {
    uart_send_string("Command '");
    uart_send_string(cmd);
    uart_send_string("' not found\n");
  }
}

void shell(void) {
  uart_send_string("type 'help' to see available commands\n");
  while (!EXIT) {
    uart_send_string("$ ");
    int read_status = read_command(cmd);
    if (!read_status)
      parse_command(cmd);
    else
      uart_send_string("\nread command failed\n");
  }
}
