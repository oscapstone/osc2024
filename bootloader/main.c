#include "shell.h"
#include "uart.h"

extern char _bootloader_relocated_addr;
extern char _start;
extern char _end;
int relocated_flag = 1;
char *_dtb;

void show_info() {
  unsigned long long sp_value;
  __asm__("mov %0, sp" : "=r"(sp_value));
  uart_send("SP reg value is: 0x%p\n", sp_value);

  unsigned long long pc_value;
  __asm__("adrp %0, 1f\n\t"
          "add %0, %0, :lo12:1f\n\t"
          "1:"
          : "=r"(pc_value));
  uart_send("PC reg value is: 0x%p\n", pc_value);
  unsigned long long code_size =
      (unsigned long long)&_end - (unsigned long long)&_start;
  uart_send("Code start: 0x%p\n", &_start);
  uart_send("Code end: 0x%p\n", &_end);
  uart_send("Code size: %l bytes\n", (unsigned long)code_size);
}

void code_relocate() {
  if (relocated_flag == 1) {
    relocated_flag = 0;
    char *start = &_start;
    char *end = &_end;
    char *relocated_addr = &_bootloader_relocated_addr;
    while (start < end) {
      *relocated_addr++ = *start++;
    }

    relocated_addr = &_bootloader_relocated_addr;
    ((void (*)(char *))relocated_addr)(_dtb);
  }
}
void main(char *arg) {
  _dtb = arg;
  uart_init();
  show_info();
  code_relocate();
  do_cmd_load_kernel();
  // shell_run();
}
