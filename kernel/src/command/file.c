#include "command/file.h"

#include "cpio.h"
#include "lib/string.h"
#include "lib/uart.h"

void list_files() {
  for (file_iter_t it = cpio_list(); it.end != 1; file_iter_next(&it)) {
    uart_println(it.entry.filename);
  }
}

void cat_file(char *args, size_t n) {
  char filename[256];
  strtok(args, filename, n, ' ');

  for (file_iter_t it = cpio_list(); it.end != 1; file_iter_next(&it)) {
    if (strncmp(it.entry.filename, filename, n) != 0) {
      continue;
    }
    uart_print(it.entry.content);
    return;
  }

  uart_printf("file not found: '%s'\n", filename);
}
