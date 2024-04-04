#include "cpio.h"

#include "string.h"

#ifdef __qemu__
#define INITRD_BASE (0x8000000)
#else
#define INITRD_BASE (0x2000000)
#endif

file_iter_t file_iter_next(const file_iter_t *cur) {
  cpio_header_t *header = (cpio_header_t *)cur->addr;
  u32_t namesize = align(hex_to_i(header->c_namesize, 8), 4);
  u32_t filesize = align(hex_to_i(header->c_filesize, 8), 4);

  char *next_path = cur->addr + sizeof(cpio_header_t);
  if (strncmp(next_path, "TRAILER!!!", 10) == 0) {
    file_iter_t it = {.end = 1};
    return it;
  }

  char *next_addr = next_path + namesize + filesize;
  file_iter_t it = {
      .addr = next_addr,
      .filename = next_addr + sizeof(cpio_header_t),
      .end = 0,
  };

  return it;
}

file_iter_t cpio_list() {
  char *addr = (char *)INITRD_BASE;
  file_iter_t begin = {.addr = addr, .filename = addr + sizeof(cpio_header_t)};

  return begin;
}
