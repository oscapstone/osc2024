#include "shell.h"

#include "alloc.h"
#include "cpio_.h"
#include "interrupt.h"
#include "mbox.h"
#include "string.h"
#include "task.h"
#include "timer.h"
#include "uart1.h"
#include "utli.h"

extern void enable_interrupt();
extern void disable_interrupt();
extern void core_timer_enable();
extern void core0_timer_interrupt_enable();
extern void core0_timer_interrupt_disable();
extern void set_core_timer_int(unsigned long long s);
extern void set_core_timer_int_sec(unsigned int s);

enum shell_status { Read, Parse };
enum ANSI_ESC { Unknown, CursorForward, CursorBackward, Delete };

enum ANSI_ESC decode_csi_key() {
  char c = uart_read();
  if (c == 'C') {
    return CursorForward;
  } else if (c == 'D') {
    return CursorBackward;
  } else if (c == '3') {
    c = uart_read();
    if (c == '~') {
      return Delete;
    }
  }
  return Unknown;
}

enum ANSI_ESC decode_ansi_escape() {
  char c = uart_read();
  if (c == '[') {
    return decode_csi_key();
  }
  return Unknown;
}

void shell_init() {
  uart_init();
  uart_flush();
  // uart_send_string("\nInit UART done\r\n");
}

void shell_input(char *cmd) {
  uart_send_string("\r# ");
  int idx = 0, end = 0, i;
  cmd[0] = '\0';
  char c;
  while ((c = uart_read()) != '\n') {
    if (c == 27) {  // Decode CSI key sequences
      enum ANSI_ESC key = decode_ansi_escape();
      switch (key) {
        case CursorForward:
          if (idx < end) idx++;
          break;
        case CursorBackward:
          if (idx > 0) idx--;
          break;
        case Delete:
          for (i = idx; i < end; i++) {  // left shift command
            cmd[i] = cmd[i + 1];
          }
          cmd[--end] = '\0';
          break;
        case Unknown:
          uart_flush();
          break;
      }
    } else if (c == 3) {  // CTRL-C
      cmd[0] = '\0';
      break;
    } else if (c == 8 || c == 127) {  // Backspace
      if (idx > 0) {
        idx--;
        for (i = idx; i < end; i++) {  // left shift command
          cmd[i] = cmd[i + 1];
        }
        cmd[--end] = '\0';
      }
    } else {
      if (idx < end) {  // right shift command
        for (i = end; i > idx; i--) {
          cmd[i] = cmd[i - 1];
        }
      }
      cmd[idx++] = c;
      cmd[++end] = '\0';
    }
    uart_send_string("\r# ");
    uart_send_string(cmd);
  }
  uart_send_string("\r\n");
}

void shell_controller(char *cmd) {
  if (!strcmp(cmd, "")) {
    return;
  }

  char *args[5];
  unsigned int i = 0, idx = 0;
  while (cmd[i] != '\0') {
    if (cmd[i] == ' ') {
      cmd[i] = '\0';
      args[idx++] = cmd + i + 1;
    }
    i++;
  }

  if (!strcmp(cmd, "help")) {
    uart_puts("help: print this help menu");
    uart_puts("hello: print Hello World!");
    uart_puts("ls: list the filenames in cpio archive");
    uart_puts(
        "cat <file name>: display the content of the speficied file included "
        "in cpio archive");
    uart_puts("malloc: get a continuous memory space");
    uart_puts("timestamp: get current timestamp");
    uart_puts("reboot: reboot the device");
    uart_puts("poweroff: turn off the device");
    uart_puts("brn: get rpi3’s board revision number");
    uart_puts("bsn: get rpi3’s board serial number");
    uart_puts("arm_mem: get ARM memory base address and size");
    uart_puts("exec: run a user program in EL0");
    uart_puts("async_uart: activate async uart I/O");
    uart_puts(
        "set_timeout <MESSAGE> <SECONDS>: print the message after given "
        "seconds");
    uart_puts("demo_preempt: for the demo of the preemption mechanism");
    uart_puts("loadimg: reupload the kernel image if the bootloader is used");

  } else if (!strcmp(cmd, "hello")) {
    uart_puts("Hello World!");
  } else if (!strcmp(cmd, "ls")) {
    cpio_ls();
  } else if (!strncmp(cmd, "cat", 3)) {
    cpio_cat(args[0]);
  } else if (!strcmp(cmd, "malloc")) {
    char *m1 = (char *)simple_malloc(8);
    if (!m1) {
      uart_puts("memory allocation fail!");
      return;
    }

    char *m2 = (char *)simple_malloc(8);
    if (!m2) {
      uart_puts("memory allocation fail!");
      return;
    }

    char *m3 = (char *)simple_malloc(1111);
    if (!m3) {
      uart_puts("memory allocation fail!");
      return;
    }

  } else if (!strcmp(cmd, "reboot")) {
    uart_puts("Rebooting...");
    reset(1000);
    while (1)
      ;  // hang until reboot
  } else if (!strcmp(cmd, "poweroff")) {
    uart_puts("Shutdown the board...");
    power_off();
  } else if (!strcmp(cmd, "brn")) {
    get_board_revision();
  } else if (!strcmp(cmd, "bsn")) {
    get_board_serial();
  } else if (!strcmp(cmd, "arm_mem")) {
    get_arm_base_memory_sz();
  } else if (!strcmp(cmd, "timestamp")) {
    print_timestamp();
  } else if (!strcmp(cmd, "exec")) {
    set_core_timer_int_sec(1);
    exec_in_el0(cpio_get_file_content_st_addr("user_prog.img"));
  } else if (!strcmp(cmd, "async_uart")) {
    enable_uart_interrupt();

    uart_puts("input 'q' to quit:");
    char c;
    while ((c = uart_read_async()) != 'q') {
      uart_write_async(c);
    }
    uart_send_string("\r\n");

    wait_usec(1500000);
    char str_buf[50];
    unsigned int n = uart_read_string_async(str_buf);
    uart_int(n);
    uart_send_string(" bytes received: ");
    uart_puts(str_buf);

    n = uart_send_string_async("12345678, ");
    wait_usec(100000);
    uart_int(n);
    uart_puts(" bytes sent");
    disable_uart_interrupt();

  } else if (!strncmp(cmd, "set_timeout", 11)) {
    char *msg = args[0];
    unsigned int sec = atoi(args[1]);

    uart_send_string("set timeout: ");
    uart_int(sec);
    uart_send_string("s, ");
    print_timestamp();

    add_timer(print_message, msg, sec);

  } else if (!strcmp(cmd, "demo_preempt")) {
    char buf[100];
    enable_uart_interrupt();
    add_task(fake_long_handler, 99);
    pop_task();
    uart_read_string_async(buf);
    uart_send_string_async(buf);
    wait_usec(100000);
    disable_uart_interrupt();
    uart_send_string("\r\n");
  } else if (!strcmp(cmd, "loadimg")) {
    asm volatile(
        "ldr x30, =0x60160;"
        "ret;");
  } else {
    uart_puts("shell: unvaild command");
  }
}

void shell_start() {
  enable_interrupt();
  core_timer_enable();
  core0_timer_interrupt_enable();
  enum shell_status status = Read;
  char cmd[CMD_LEN];
  while (1) {
    switch (status) {
      case Read:
        shell_input(cmd);
        status = Parse;
        break;

      case Parse:
        shell_controller(cmd);
        status = Read;
        break;
    }
  }
};
