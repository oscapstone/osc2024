#include "shell.h"
#include "cpio.h"
#include "dtb.h"
#include "heap.h"
#include "mbox.h"
#include "power.h"
#include "timer.h"
#include "u_string.h"
#include "uart1.h"
#include <stddef.h>

#define CLI_MAX_CMD 11
#define USTACK_SIZE 0x10000

extern char *dtb_ptr;
void *CPIO_DEFAULT_PLACE;
extern int  preempt;

void cli_cmd_clear(char *buffer, int length) {
  for (int i = 0; i < length; i++) {
    buffer[i] = '\0';
  }
};

void cli_cmd_read(char *buffer) {
  char c = '\0';
  int idx = 0;
  while (1) {
    if (idx >= CMD_MAX_LEN)
      break;
    c = uart_async_getc();
    if (c == '\n')
      break;
    buffer[idx++] = c;
  }
}

void cli_cmd_exec(char *buffer) {
  if (!buffer)
    return;

  char *cmd = buffer;
  char *argvs = str_SepbySpace(buffer);

  if (strcmp(cmd, "cat") == 0) {
    do_cmd_cat(argvs);
  } else if (strcmp(cmd, "dtb") == 0) {
    do_cmd_dtb();
  } else if (strcmp(cmd, "exec") == 0) {
    do_cmd_exec(argvs);
  } else if (strcmp(cmd, "hello") == 0) {
    do_cmd_hello();
  } else if (strcmp(cmd, "help") == 0) {
    do_cmd_help();
  } else if (strcmp(cmd, "info") == 0) {
    do_cmd_info();
  } else if (strcmp(cmd, "kmalloc") == 0) {
    do_cmd_kmalloc();
  } else if (strcmp(cmd, "ls") == 0) {
    do_cmd_ls(argvs);
  } else if (strcmp(cmd, "setTimeout") == 0) {
    char *sec = str_SepbySpace(argvs);
    do_cmd_setTimeout(argvs, sec);
  } else if (strcmp(cmd, "set2sAlert") == 0) {
    do_cmd_set2sAlert();
  } else if (strcmp(cmd, "reboot") == 0) {
    do_cmd_reboot();
  } else if (strcmp(cmd, "preempt") == 0) {
    do_cmd_preempt();
  }
}

void cli_print_banner() {
  uart_puts("\r\n");
  uart_puts("=======================================\r\n");
  uart_puts("                 Shell                 \r\n");
  uart_puts("=======================================\r\n");
}

void do_cmd_cat(char *filepath) {
  char *c_filepath;
  char *c_filedata;
  unsigned int c_filesize;
  struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

  while (header_ptr != 0) {
    int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize,
                                       &c_filedata, &header_ptr);
    // if parse header error
    if (error) {
      uart_puts("cpio parse error");
      break;
    }

    if (strcmp(c_filepath, filepath) == 0) {
      uart_puts("%s", c_filedata);
      break;
    }

    // if this is TRAILER!!! (last of file)
    if (header_ptr == 0)
      uart_puts("cat: %s: No such file or directory\n", filepath);
  }
}

void do_cmd_dtb() { traverse_device_tree(dtb_ptr, dtb_callback_show_tree); }

void do_cmd_help() {
  uart_puts("\n");
  uart_puts("\tcat:\t\tconcatenate files and print on the standard output.\n");
  uart_puts("\tdtb:\t\tshow device tree.\n");
  uart_puts("\texec:\t\tturn program from cpio.\n");
  uart_puts("\thello:\t\tprint Hello World!.\n");
  uart_puts("\thelp:\t\tprint all available commands.\n");
  uart_puts("\tkmalloc:\tsimple allocator in heap session.\n");
  uart_puts("\tinfo:\t\tget device information via mailbox.\n");
  uart_puts("\tls:\t\tlist directory contents.\n");
  uart_puts("\tsetTimeout:\tsetTimeout [MESSAGE] [SECONDS].\n");
  uart_puts("\tset2sAlert:\tset core timer interrupt every 2 second.\n");
  uart_puts("\treboot:\t\treboot the device.\n");
  uart_puts("\tpreempt:\t\tdemo preemption.\n");
  uart_puts("\n");
}

void do_cmd_exec(char *filepath) {
  char *c_filepath;
  char *c_filedata;
  unsigned int c_filesize;
  struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

  while (header_ptr != 0) {
    int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize,
                                       &c_filedata, &header_ptr);
    // if parse header error
    if (error) {
      uart_puts("cpio parse error");
      break;
    }

    if (strcmp(c_filepath, filepath) == 0) {
      // exec c_filedata
      char *ustack = kmalloc(USTACK_SIZE);
      //uart_2hex((unsigned int)(unsigned long)c_filedata);
      asm("msr elr_el1, %0\n\t"   // elr_el1: Set the address to return to:
                                  // c_filedata
          "msr spsr_el1, xzr\n\t" // enable interrupt (PSTATE.DAIF) ->
                                  // spsr_el1[9:6]=4b0. In Basic#1 sample, EL1
                                  // interrupt is disabled.
          "msr sp_el0, %1\n\t" // user program stack pointer set to new stack.
          "eret\n\t"           // Perform exception return. EL1 -> EL0
          ::"r"(c_filedata),
          "r"(ustack + USTACK_SIZE));
      free(ustack);
      break;
    }

    // if this is TRAILER!!! (last of file)
    if (header_ptr == 0)
      uart_puts("cat: %s: No such file or directory\n", filepath);
  }
}

void do_cmd_hello() { uart_puts("Hello World!\r\n"); }

void do_cmd_info() {
  // print hw revision
  pt[0] = 8 * 4;
  pt[1] = MBOX_REQUEST_PROCESS;
  pt[2] = MBOX_TAG_GET_BOARD_REVISION;
  pt[3] = 4;
  pt[4] = MBOX_TAG_REQUEST_CODE;
  pt[5] = 0;
  pt[6] = 0;
  pt[7] = MBOX_TAG_LAST_BYTE;

  if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt))) {
    uart_puts("Hardware Revision\t: ");
    uart_2hex(pt[6]);
    uart_2hex(pt[5]);
    uart_puts("\r\n");
  }
  // print arm memory
  pt[0] = 8 * 4;
  pt[1] = MBOX_REQUEST_PROCESS;
  pt[2] = MBOX_TAG_GET_ARM_MEMORY;
  pt[3] = 8;
  pt[4] = MBOX_TAG_REQUEST_CODE;
  pt[5] = 0;
  pt[6] = 0;
  pt[7] = MBOX_TAG_LAST_BYTE;

  if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt))) {
    uart_puts("ARM Memory Base Address\t: ");
    uart_2hex(pt[5]);
    uart_puts("\r\n");
    uart_puts("ARM Memory Size\t\t: ");
    uart_2hex(pt[6]);
    uart_puts("\r\n");
  }
}

void do_cmd_kmalloc() {
  // test malloc
  char *test1 = kmalloc(0x18);
  memcpy(test1, "test malloc1", sizeof("test malloc1"));
  uart_puts("%s\n", test1);

  char *test2 = kmalloc(0x20);
  memcpy(test2, "test malloc2", sizeof("test malloc2"));
  uart_puts("%s\n", test2);

  char *test3 = kmalloc(0x28);
  memcpy(test3, "test malloc3", sizeof("test malloc3"));
  uart_puts("%s\n", test3);
}

void do_cmd_ls(char *workdir) {
  char *c_filepath;
  char *c_filedata;
  unsigned int c_filesize;
  struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

  while (header_ptr != 0) {
    int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize,
                                       &c_filedata, &header_ptr);
    // if parse header error
    if (error) {
      uart_puts("cpio parse error");
      break;
    }

    // if this is not TRAILER!!! (last of file)
    if (header_ptr != 0)
      uart_puts("%s\n", c_filepath);
  }
}

void do_cmd_setTimeout(char *msg, char *sec) {
  add_timer(uart_sendline, atoi(sec), msg);
}

void do_cmd_set2sAlert() { add_timer(timer_set2sAlert, 2, "2sAlert"); }

void do_cmd_reboot() {
  uart_puts("Reboot in 5 seconds ...\r\n\r\n");
  volatile unsigned int *rst_addr = (unsigned int *)PM_RSTC;
  *rst_addr = PM_PASSWORD | 0x20;
  volatile unsigned int *wdg_addr = (unsigned int *)PM_WDOG;
  *wdg_addr = PM_PASSWORD | 5;
}

void do_cmd_preempt() {
  add_timer(test1, 5, NULL);
  add_timer(test2, 10, NULL);
}

void test1(char *arg) {
  uart_sendline("preempt_test1\n");
  preempt = 1;
  while (preempt) {
  }
}

void test2(char *arg) {
  uart_sendline("preempt_test2\n");
  preempt = 0;
}
