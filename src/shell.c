#include "alloc.h"
#include "cpio_.h"
#include "mbox.h"
#include "my_string.h"
#include "uart1.h"
#include "utli.h"

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
  uart_send_string("\nInit UART done\r\n");
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
    // uart_printf("\r# %s \r\e[%dC", cmd, idx + 2);
    uart_send_string("\r# ");
    uart_send_string(cmd);
  }
  uart_send_string("\r\n");
}

void shell_controller(char *cmd) {
  if (!strcmp(cmd, "")) {
    return;
  } else if (!strcmp(cmd, "help")) {
    uart_puts("help: print this help menu");
    uart_puts("hello: print Hello World!");
    uart_puts("ls: list the filenames in cpio archive");
    uart_puts(
        "cat: display the content of the speficied file included in cpio "
        "archive");
    uart_puts("malloc: get a continuous memory space");
    uart_puts("timestamp: get current timestamp");
    uart_puts("reboot: reboot the device");
    uart_puts("poweroff: turn off the device");
    uart_puts("brn: get rpi3’s board revision number");
    uart_puts("bsn: get rpi3’s board serial number");
    uart_puts("arm_mem: get ARM memory base address and size");
    uart_puts("loadimg: reupload the kernel image if the bootloader is used");
  } else if (!strcmp(cmd, "hello")) {
    uart_puts("Hello World!");
  } else if (!strcmp(cmd, "ls")) {
    cpio_ls();
  } else if (!strncmp(cmd, "cat", 3)) {
    cpio_cat(cmd + 4);
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
    uart_int(get_timestamp());
    uart_send_string("\r\n");
  } else if (!strcmp(cmd, "loadimg")) {
    asm volatile(
        "ldr x30, =0x60160;"
        "ret;");
  } else {
    uart_puts("shell: unvaild command");
  }
}
