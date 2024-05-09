#include "dtb.h"
#include "shell.h"
#include "uart.h"

extern char *dtb_ptr;

int main(char *arg) {
  dtb_ptr = arg;
  uart_init();
  uart_send("DTB header at address 0x%p\n", dtb_ptr);
  dtb_find_initramfs_start(dtb_ptr);
  shell_run();

  return 0;
}