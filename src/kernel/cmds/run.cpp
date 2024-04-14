#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "exec.hpp"
#include "fs/initramfs.hpp"

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

  exec_user_prog(__user_text, __user_stack);

  return 0;
}
