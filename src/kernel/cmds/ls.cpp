#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "cpio.hpp"

void cmd_ls() {
  for (auto it : cpio) {
    mini_uart_printf("%c\t%d\t%s\n", "-d"[it->isdir()], it -> filesize(),
                     it -> name_ptr());
  }
}
