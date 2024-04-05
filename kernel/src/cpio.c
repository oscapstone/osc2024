#include "cpio.h"

#include "string.h"

#ifdef __QEMU__
#define INITRD_BASE (0x8000000)
#else
#define INITRD_BASE (0x2000000)
#endif

file_iter_t file_iter_next(const file_iter_t *cur) {
  cpio_header_t *header = (cpio_header_t *)cur->addr;

  u32_t namesize = hex_to_i(header->c_namesize, 8);
  u32_t header_name_size = align(sizeof(cpio_header_t) + namesize, 4);

  u32_t filesize = align(hex_to_i(header->c_filesize, 8), 4);
  size_t header_size = header_name_size + filesize;

  cpio_header_t *next_header = (cpio_header_t *)(cur->addr + header_size);

  char *next_path = (char *)(next_header + 1);
  if (strncmp(next_path, "TRAILER!!!", 10) == 0) {
    file_iter_t it = {.end = 1};
    return it;
  }

  file_iter_t it = {
      .addr = (char *)next_header,
      .filename = (char *)(next_header + 1),
      .end = 0,
  };

  return it;
}

file_iter_t cpio_list() {
  char *addr = (char *)INITRD_BASE;
  file_iter_t begin = {.addr = addr, .filename = addr + sizeof(cpio_header_t)};

  return begin;
}
