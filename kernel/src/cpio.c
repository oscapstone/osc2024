#include "cpio.h"

#include "initrd.h"
#include "lib/string.h"

void file_iter_next(file_iter_t *cur) {
  cpio_header_t *header = (cpio_header_t *)cur->next_addr;
  cpio_stat_t stat = parse_header(header);

  cur->next_addr +=
      sizeof(cpio_header_t) + stat.filename_size + stat.content_size;

  cur->entry = (cpio_entry_t){
      .filename = stat.filename,
      .content = stat.content,
  };

  cur->end = strncmp(stat.filename, "TRAILER!!!", 10) == 0;
}

file_iter_t cpio_list() {
  char *addr = initrd_base;

  cpio_stat_t stat = parse_header((cpio_header_t *)addr);
  file_iter_t begin = {
      .next_addr =
          addr + sizeof(cpio_header_t) + stat.filename_size + stat.content_size,
      .entry = {.filename = stat.filename, .content = stat.content},
      .end = strncmp(stat.filename, "TRAILER!!!", 10) == 0,
  };

  return begin;
}

cpio_stat_t parse_header(cpio_header_t *header) {
  u32_t namesize = hex_to_i(header->c_namesize, 8);
  u32_t header_name_size = align(sizeof(cpio_header_t) + namesize, 4);

  u32_t filesize = align(hex_to_i(header->c_filesize, 8), 4);

  return (cpio_stat_t){
      .filename = (char *)(header + 1),
      .filename_size = header_name_size - sizeof(cpio_header_t),
      .content = (char *)(header) + header_name_size,
      .content_size = filesize,
  };
}
