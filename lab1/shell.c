#include "shell.h"
#include "mbox.h"
#include "power.h"
#include "uart.h"
#include "utils.h"

void show_banner() {
  uart_send("\n\n\r\x1B[2J\x1B[H");
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
      if (c < 0 || c >= 128)
        continue;

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
      show_banner();
    } else if (strcmp(cmd, "reboot") == 0) {
      reboot();
    } else if (strcmp(cmd, "cancel") == 0) {
      cancel_reboot();
    } else if (strcmp(cmd, "exit") == 0) {
      uart_send("Exiting...\n");
      break;
    } else if (end_pos > 0) {
      uart_send("Unknown command: %s\n", cmd);
    }
  }
}

void do_cmd_help() {
  uart_send("Supported commands:\n");
  uart_send("\x1B[32m  help    - Display all available commands.\n");
  uart_send("  hello   - Display 'Hello, World!'\n");
  uart_send("  info    - Display board revision and ARM memory information"
            "via mailbox.\n");
  uart_send("  reboot  - Reboot the device.\n");
  uart_send("  cancel  - Cancel the ongoing reboot.\n");
  uart_send("  clear   - Clear the screen.\n");
  uart_send("  exit    - Exit the shell.\x1B[0m\n");
}

void do_cmd_info() {
  get_board_revision();
  get_arm_memory();
}