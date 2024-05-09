#include "shell.h"
#include "dtb.h"
#include "heap.h"
#include "include/cpio.h"
#include "mbox.h"
#include "power.h"
#include "uart.h"
#include "utils.h"

extern cpio_newc_header *cpio_header;
extern char *dtb_ptr;

void show_banner() {
  uart_send("======================================================\n");
  uart_send("||                                                  ||\n");
  uart_send("||           Raspberry Pi Mini UART Shell           ||\n");
  uart_send("||                                                  ||\n");
  uart_send("======================================================\n");
  uart_send("Type 'help' for a list of available commands.\n");
}

void shell_run() {
  char cmd[MAX_CMD_LEN];
  int cur_pos = 0, end_pos = 0;
  show_banner();

  while (1) {
    uart_send("# ");
    end_pos = 0, cur_pos = 0;
    char c = '\0';
    for (int i = 0; i < MAX_CMD_LEN; i++) {
      cmd[i] = '\0';
    }

    do {
      c = uart_getc();
      // if (c < 0 || c >= 128)
      //   continue;

      if (c == '\n') {
        uart_send("\n");
        cmd[end_pos] = '\0';
        break;
      } else if (c == '\b' || c == 0x7F) {
        if (cur_pos > 0) {
          cur_pos--, end_pos--;
          char *start_ptr = &cmd[cur_pos];
          char *end_ptr = &cmd[end_pos] - 1;
          for (char *ptr = start_ptr; ptr <= end_ptr; ptr++) {
            *ptr = *(ptr + 1);
          }
          cmd[end_pos] = '\0';
          uart_send("\b\x1b[s\x1b[K%s\x1b[u", &cmd[cur_pos]);
        }
      } else if (c == '\x1b') {
        c = uart_getc(), c = uart_getc();
        if (c == 'D' && cur_pos > 0) {
          cur_pos--;
          uart_send("\x1b[D");
        } else if (c == 'C' && cur_pos < end_pos) {
          cur_pos++;
          uart_send("\x1b[C");
        }
      } else {
        char *end_ptr = &cmd[end_pos] - 1;
        char *insert_ptr = &cmd[cur_pos];
        for (char *ptr = end_ptr; ptr >= insert_ptr; ptr--) {
          *(ptr + 1) = *ptr;
        }
        *insert_ptr = c;
        cur_pos++, end_pos++;
        uart_send("\x1b[s\x1b[K%s\x1b[u\x1b[C", insert_ptr);
      }
    } while (end_pos < MAX_CMD_LEN - 1);

    if (strcmp(cmd, "help") == 0) {
      do_cmd_help();
    } else if (strcmp(cmd, "hello") == 0) {
      uart_send("Hello, World!\n");
    } else if (strcmp(cmd, "info") == 0) {
      do_cmd_info();
    } else if (strcmp(cmd, "clear") == 0) {
      do_cmd_clear();
    } else if (strcmp(cmd, "reboot") == 0) {
      reboot();
    } else if (strcmp(cmd, "cancel") == 0) {
      cancel_reboot();
    } else if (strcmp(cmd, "ls") == 0) {
      do_cmd_ls();
    } else if (strncmp(cmd, "cat ", 4) == 0) {
      do_cmd_cat(&cmd[4]);
    } else if (strcmp(cmd, "malloc") == 0) {
      do_cmd_malloc();
    } else if (strcmp(cmd, "dtb") == 0) {
      do_cmd_dtb();
    } else if (strcmp(cmd, "exit") == 0) {
      uart_send("Exiting...\n");
      break;
    } else if (end_pos > 0) {
      uart_send("Unknown command: %s\n", cmd);
    }
  }
}

void format_command(const char *command, const char *description) {
  int command_width = 18;
  int len = strlen(command);
  uart_send(command);
  for (int i = len; i < command_width; i++) {
    uart_send(" ");
  }
  uart_send("-  %s\n", description);
}

void do_cmd_help() {
  uart_send("Supported commands:\n");
  uart_send("\x1B[32m");
  format_command("help", "Display all available commands.");
  format_command("hello", "Display 'Hello, World!'");
  format_command(
      "info", "Display board revision and ARM memory information via mailbox.");
  format_command("reboot", "Reboot the device.");
  format_command("cancel", "Cancel the ongoing reboot.");
  format_command("clear", "Clear the screen.");
  format_command("ls", "List directory contents.");
  format_command("cat <filename>", "Display content of a file.");
  format_command("malloc", "Demonstrate memory allocation examples.");
  format_command("dtb", "Display the device tree structure and details.");
  format_command("exit", "Exit the shell.");
  uart_send("\x1B[0m");
}

void do_cmd_info() {
  get_board_revision();
  get_arm_memory();
}

void do_cmd_clear() {
  uart_send("\n\n\r\x1B[2J\x1B[H");
  show_banner();
}

void do_cmd_ls() {
  if (cpio_header == NULL) {
    uart_send("CPIO header is NULL.\n");
    return;
  }
  parse_cpio(cpio_header, NULL);
}

void do_cmd_cat(const char *args) {
  if (cpio_header == NULL) {
    uart_send("CPIO header is NULL.\n");
    return;
  }
  if (args == NULL || *args == '\0') {
    uart_send("Usage: cat <filename>\n");
    return;
  }
  if (parse_cpio(cpio_header, args) == -1) {
    uart_send("File %s not found.\n", args);
  }
}

void do_cmd_malloc() {
  if (simple_malloc(0x7) == NULL) {
    uart_send("Error: Out of memory.\n");
  }
  if (simple_malloc(0x17) == NULL) {
    uart_send("Error: Out of memory.\n");
  }
  if (simple_malloc(0x28) == NULL) {
    uart_send("Error: Out of memory.\n");
  }
  if (simple_malloc(0x10000) == NULL) {
    uart_send("Error: Out of memory.\n");
  }
  if (simple_malloc(0x100000) == NULL) {
    uart_send("Error: Out of memory.\n");
  }
}

void do_cmd_dtb() { traverse_device_tree(dtb_ptr); }