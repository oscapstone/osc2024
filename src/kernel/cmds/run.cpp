#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "initramfs.hpp"

extern char __user_text[];
extern char __user_stack[];

int cmd_run(int argc, char* argv[]) {
  if (argc != 2) {
    mini_uart_puts("run: require one argument\n");
    mini_uart_puts("usage: run <program>\n");
    return -1;
  }

  auto name = argv[1];
  auto hdr = initramfs.find(name);
  if (hdr == nullptr) {
    mini_uart_printf("run: %s: No such file or directory\n", name);
    return -1;
  }
  if (hdr->isdir()) {
    mini_uart_printf("run: %s: Is a directory\n", name);
    return -1;
  }

  auto file = hdr->file();
  memcpy(__user_text, file.data(), file.size());

  asm volatile(
      "mov x0, 0x3c0\n"
      "msr SPSR_EL1, x0\n"
      "mov x0, %0\n"
      "msr SP_EL0, x0\n"
      "mov x0, %1\n"
      "msr ELR_EL1, x0\n"
      "eret\n" ::"r"(__user_stack),
      "r"(__user_text));

  return 0;
}
