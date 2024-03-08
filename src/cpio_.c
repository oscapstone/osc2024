#include "cpio_.h"

#include "my_string.h"
#include "uart0.h"
#include "utli.h"

// char *cpio_addr = (char *)0x20000000;
char *cpio_addr;

static unsigned int atoi(const char *s, int char_size) {
  unsigned int num = 0;
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
  char *addr = cpio_addr;
  while (strcmp((char *)(addr + sizeof(struct cpio_newc_header)),
                "TRAILER!!!") != 0) {
    cpio_header *header = (cpio_header *)addr;
    unsigned int filename_size =
        atoi(header->c_namesize, (int)sizeof(header->c_namesize));
    unsigned int headerPathname_size = sizeof(cpio_header) + filename_size;
    unsigned int file_size =
        atoi(header->c_filesize, (int)sizeof(header->c_filesize));
    align_inplace(&headerPathname_size, 4);
    align_inplace(&file_size, 4);
    uart_printf("%s\n", addr + sizeof(cpio_header));
    addr += (headerPathname_size + file_size);
  }
}

char *findFile(const char *name) {
  char *addr = cpio_addr;
  while (strcmp((char *)(addr + sizeof(struct cpio_newc_header)),
                "TRAILER!!!") != 0) {
    if (!strcmp((char *)(addr + sizeof(cpio_header)), name)) {
      return addr;
    }
    cpio_header *header = (cpio_header *)addr;
    unsigned int pathname_size =
        atoi(header->c_namesize, (int)sizeof(header->c_namesize));
    unsigned int file_size =
        atoi(header->c_filesize, (int)sizeof(header->c_filesize));
    unsigned int headerPathname_size = sizeof(cpio_header) + pathname_size;
    align_inplace(&headerPathname_size, 4);
    align_inplace(&file_size, 4);
    addr += (headerPathname_size + file_size);
  }
  return 0;
}

void cpio_cat(const char *filename) {
  char *file = findFile(filename);
  if (file) {
    cpio_header *header = (cpio_header *)file;
    unsigned int filename_size =
        atoi(header->c_namesize, (int)sizeof(header->c_namesize));
    unsigned int headerPathname_size = sizeof(cpio_header) + filename_size;
    unsigned int file_size =
        atoi(header->c_filesize, (int)sizeof(header->c_filesize));
    align_inplace(&headerPathname_size, 4);
    align_inplace(&file_size, 4);
    char *file_content = (char *)header + headerPathname_size;
    for (unsigned int i = 0; i < file_size; i++) {
      uart_write(file_content[i]);
    }
  } else {
    uart_printf("File \"%s\" not found\n", filename);
  }
}