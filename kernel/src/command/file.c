#include "command/file.h"

#include "cpio.h"
#include "uart.h"

void list_files() {
  for (file_iter_t it = cpio_list(); it.end != 1; it = file_iter_next(&it)) {
    uart_println(it.filename);
  }
}
