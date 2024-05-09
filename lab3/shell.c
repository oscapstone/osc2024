#include "include/shell.h"
#include "include/cpio.h"
#include "include/dtb.h"
#include "include/exception.h"
#include "include/heap.h"
#include "include/mbox.h"
#include "include/power.h"
#include "include/timer.h"
#include "include/types.h"
#include "include/uart.h"
#include "include/utils.h"

extern cpio_newc_header_t *cpio_header;
extern char *dtb_ptr;
extern char cmd[];
extern int preempt;

void show_banner() {
  uart_sendline("======================================================\n");
  uart_sendline("||                                                  ||\n");
  uart_sendline("||           Raspberry Pi Mini UART Shell           ||\n");
  uart_sendline("||                                                  ||\n");
  uart_sendline("======================================================\n");
  uart_sendline("Type 'help' for a list of available commands.\n");
}

void shell_run() {
  char *saveptr;
  int cur_pos = 0, end_pos = 0;
  show_banner();

  while (1) {
    uart_sendline("# ");
    end_pos = 0, cur_pos = 0;
    char c = '\0';
    for (int i = 0; i < MAX_CMD_LEN; ++i) {
      cmd[i] = '\0';
    }

    do {
      c = uart_async_getc();
      if (c == '\n') {
        uart_sendline("\n");
        cmd[end_pos] = '\0';
        break;
      } else if (c == '\b' || c == 0x7F) {
        if (cur_pos > 0) {
          cur_pos--, end_pos--;
          char *start_ptr = &cmd[cur_pos];
          char *end_ptr = &cmd[end_pos] - 1;
          for (char *ptr = start_ptr; ptr <= end_ptr; ++ptr) {
            *ptr = *(ptr + 1);
          }
          cmd[end_pos] = '\0';
          uart_sendline("\b\x1b[s\x1b[K%s\x1b[u", &cmd[cur_pos]);
        }
      } else if (c == '\x1b') {
        c = uart_async_getc(), c = uart_async_getc();
        if (c == 'D' && cur_pos > 0) {
          cur_pos--;
          uart_sendline("\x1b[D");
        } else if (c == 'C' && cur_pos < end_pos) {
          cur_pos++;
          uart_sendline("\x1b[C");
        }
      } else {
        char *end_ptr = &cmd[end_pos] - 1;
        char *insert_ptr = &cmd[cur_pos];
        for (char *ptr = end_ptr; ptr >= insert_ptr; --ptr) {
          *(ptr + 1) = *ptr;
        }
        *insert_ptr = c;
        cur_pos++, end_pos++;
        uart_sendline("\x1b[s\x1b[K%s\x1b[u\x1b[C", insert_ptr);
      }
    } while (end_pos < MAX_CMD_LEN - 1);

    char *token = strtok(cmd, " ", &saveptr);
    if (token == NULL)
      continue;
    if (strcmp(token, "help") == 0) {
      do_cmd_help();
    } else if (strcmp(token, "hello") == 0) {
      char *name = "Jim";
      uart_async_sendline("Async Hello, %s!\r\n", name);
      // uart_send("Hello, World!\n");
    } else if (strcmp(token, "info") == 0) {
      do_cmd_info();
    } else if (strcmp(token, "clear") == 0) {
      do_cmd_clear();
    } else if (strcmp(token, "reboot") == 0) {
      reboot();
    } else if (strcmp(token, "cancel") == 0) {
      abort_reboot();
    } else if (strcmp(token, "ls") == 0) {
      do_cmd_ls();
    } else if (strcmp(token, "cat") == 0) {
      char *filename = strtok(NULL, " ", &saveptr);
      if (filename) {
        do_cmd_cat(filename);
      } else {
        uart_sendline("Usage: cat <filename>\n");
      }
    } else if (strcmp(token, "malloc") == 0) {
      do_cmd_malloc();
    } else if (strcmp(token, "dtb") == 0) {
      do_cmd_dtb();
    } else if (strcmp(token, "exec") == 0) {
      do_cmd_exec("./example.img");
      // char *progname = strtok(NULL, " ", &saveptr);
      // if (progname) {
      //   do_cmd_exec(progname);
      // } else {
      //   uart_send("Usage: exec <program_name>\n");
      // }
    } else if (strcmp(token, "set") == 0) {
      char *msg = strtok(NULL, " ", &saveptr);
      char *secs = strtok(NULL, " ", &saveptr);
      char *priority = strtok(NULL, " ", &saveptr);
      if (msg && secs) {
        if (priority)
          do_cmd_setTimeout(msg, atoi(secs), TIMER_IRQ_DEFAULT_PRIORITY);
        else
          do_cmd_setTimeout(msg, atoi(secs), atoi(priority));
      } else {
        uart_sendline("Usage: set <message> <seconds> [priority]\n");
      }
    } else if (strcmp(token, "preempt") == 0) {
      do_cmd_preempt();
    } else if (strcmp(token, "exit") == 0) {
      uart_sendline("Exiting...\n");
      break;
    } else {
      uart_sendline("Unknown command: %s\n", cmd);
    }
  }
}

void format_command(const char *command, const char *description) {
  int command_width = 30;
  int len = strlen(command);
  uart_sendline(command);
  for (int i = len; i < command_width; ++i) {
    uart_sendline(" ");
  }
  uart_sendline("-  %s\n", description);
}

void do_cmd_help() {
  uart_sendline("Supported commands:\n");
  uart_sendline("\x1B[32m");
  format_command("help", "Show available commands.");
  format_command("hello", "Print 'Hello, World!'");
  format_command("info", "Show board and memory info.");
  format_command("reboot", "Reboot the device.");
  format_command("cancel", "Cancel an ongoing reboot.");
  format_command("clear", "Clear the screen.");
  format_command("ls", "List directory contents.");
  format_command("cat <filename>", "Display a file's content.");
  format_command("malloc", "Demonstrate memory allocation.");
  format_command("dtb", "Show device tree structure.");
  format_command("exec", "Execute the user program.");
  format_command("set <msg> <secs> [priority]", "Schedule a timer message.");
  format_command("preempt", "Test reemptive irq.");
  format_command("exit", "Exit the shell.");
  uart_sendline("\x1B[0m");
}

void do_cmd_info() {
  mbox_get_board_revision();
  mbox_get_arm_memory();
}

void do_cmd_clear() {
  uart_sendline("\n\n\r\x1B[2J\x1B[H");
  show_banner();
}

void do_cmd_ls() {
  if (cpio_header == NULL) {
    uart_sendline("CPIO header is NULL.\n");
    return;
  }
  cpio_parse_header(cpio_header, NULL);
}

void do_cmd_cat(const char *filename) {
  if (cpio_header == NULL) {
    uart_sendline("CPIO header is NULL.\n");
    return;
  }
  if (cpio_parse_header(cpio_header, filename) == -1) {
    uart_sendline("File %s not found.\n", filename);
  }
}

void do_cmd_malloc() {
  simple_malloc(0x7, 1);
  simple_malloc(0x17, 1);
  simple_malloc(0x28, 1);
  simple_malloc(0x10000, 1);
  simple_malloc(0x100000, 1);
}

void do_cmd_dtb() { dtb_traverse_device_tree(dtb_ptr); }

void do_cmd_exec(const char *progname) {
  char *program_address = cpio_extract_file_address(progname);
  if (!program_address) {
    uart_sendline("Failed to locate the program file %s in the CPIO archive.\n",
                  progname);
    return;
  }
  unsigned int user_stack_size = 0x1000;
  char *user_stack = simple_malloc(user_stack_size, 1);
  if (!user_stack) {
    uart_sendline("Failed to allocate memory for user stack.\n");
    return;
  }
  char *user_stack_top = user_stack + user_stack_size;
  add_timer_task(create_timer_task(5, back_to_kernel,
                                   "Continuing kernel execution...\n", 0));
  uart_sendline("Executing user program...\n");
  asm volatile("msr sp_el0, %0\n"
               "msr elr_el1, %1\n"
               "msr spsr_el1, %2\n"
               "eret\n"
               :
               : "r"(user_stack_top), "r"(program_address), "r"(0x00000000)
               : "memory");
}

void back_to_kernel(char *arg) {
  uart_sendline("%s", arg);
  el1_interrupt_enable();
  shell_run();
}

void do_cmd_setTimeout(const char *msg, int secs, int priority) {
  uart_sendline("set message = '%s', delay = %d, priority = %d\n", msg, secs,
                priority);
  add_timer_task(create_timer_task(secs, uart_sendline, msg, priority));
}

void do_cmd_preempt() {
  add_timer_task(create_timer_task(5, start_preemption_test, NULL, 1));
  add_timer_task(create_timer_task(10, stop_preemption_test, NULL, 0));
}

void start_preemption_test(char *arg) {
  uart_sendline("Starting preemption test.\n");
  preempt = 1;
  while (preempt) {
  }
}

void stop_preemption_test(char *arg) {
  uart_sendline("Stopping preemption test.\n");
  preempt = 0;
}