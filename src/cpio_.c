#include "cpio_.h"

#include "string.h"
#include "uart1.h"
#include "utli.h"
#include "vm.h"
#include "vm_macro.h"

void *cpio_start_addr;
void *cpio_end_addr;

static uint32_t cpio_atoi(const char *s, int32_t char_size) {
  uint32_t num = 0;
  for (int i = 0; i < char_size; i++) {
    num = num * 16;
    if (*s >= '0' && *s <= '9') {
      num += (*s - '0');
    } else if (*s >= 'A' && *s <= 'F') {
      num += (*s - 'A' + 10);
    } else if (*s >= 'a' && *s <= 'f') {
      num += (*s - 'a' + 10);
    }
    s++;
  }
  return num;
}

void cpio_ls() {
  void *addr = cpio_start_addr;
  while (strcmp((char *)(addr + sizeof(cpio_header)), "TRAILER!!!") != 0) {
    cpio_header *header = (cpio_header *)addr;
    uint32_t filename_size =
        cpio_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
    uint32_t headerPathname_size = sizeof(cpio_header) + filename_size;
    uint32_t file_size =
        cpio_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
    align_inplace(&headerPathname_size, 4);
    align_inplace(&file_size, 4);
    uart_send_string(addr + sizeof(cpio_header));
    uart_send_string("  ");
    uart_int(file_size);
    uart_puts("B");
    addr += (headerPathname_size + file_size);
  }
}

char *findFile(const char *name) {
  void *addr = cpio_start_addr;
  while (strcmp((char *)(addr + sizeof(cpio_header)), "TRAILER!!!") != 0) {
    if (!strcmp((char *)(addr + sizeof(cpio_header)), name)) {
      return addr;
    }
    cpio_header *header = (cpio_header *)addr;
    uint32_t pathname_size =
        cpio_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
    uint32_t file_size =
        cpio_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
    uint32_t headerPathname_size = sizeof(cpio_header) + pathname_size;
    align_inplace(&headerPathname_size, 4);
    align_inplace(&file_size, 4);
    addr += (headerPathname_size + file_size);
  }
  return (char *)0;
}

void cpio_cat(const char *filename) {
  char *file = findFile(filename);
  if (file) {
    cpio_header *header = (cpio_header *)file;
    uint32_t filename_size =
        cpio_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
    uint32_t headerPathname_size = sizeof(cpio_header) + filename_size;
    uint32_t file_size =
        cpio_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
    align_inplace(&headerPathname_size, 4);
    align_inplace(&file_size, 4);
    char *file_content = (char *)header + headerPathname_size;
    for (uint32_t i = 0; i < file_size; i++) {
      uart_write(file_content[i]);
    }
    uart_send_string("\r\n");
  } else {
    uart_send_string(filename);
    uart_puts(" not found");
  }
}

char *cpio_load(const char *filename, uint32_t *file_sz) {
  char *file = (void *)phy2vir((uint64_t)findFile(filename));
  if (file) {
    cpio_header *header = (cpio_header *)file;
    uint32_t filename_size =
        cpio_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
    uint32_t headerPathname_size = sizeof(cpio_header) + filename_size;
    *file_sz = cpio_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
    align_inplace(&headerPathname_size, 4);
    align_inplace(file_sz, 4);
    char *file_content = (char *)header + headerPathname_size;
    return file_content;
  }
  *file_sz = 0;
  uart_send_string(filename);
  uart_puts(" not found");
  return (char *)0;
}