#include "include/cpio.h"
#include "include/heap.h"
#include "include/types.h"
#include "include/uart.h"
#include "include/utils.h"

extern cpio_newc_header_t *cpio_header;

unsigned long cpio_hexstr_2_ulong(const char *hex, int length) {
  unsigned long result = 0;
  for (int i = 0; i < length; ++i) {
    result <<= 4;
    char c = hex[i];
    if (c >= '0' && c <= '9')
      result += c - '0';
    else if (c >= 'A' && c <= 'F')
      result += c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
      result += c - 'a' + 10;
  }
  return result;
}

int cpio_is_directory(const char *mode) {
  unsigned long mode_val = cpio_hexstr_2_ulong(mode, 8);
  return (mode_val & 0170000) == 0040000;
}

int cpio_parse_header(cpio_newc_header_t *header, const char *filename) {
  while (header != NULL) {
    if (strncmp(header->c_magic, CPIO_NEWC_MAGIC, sizeof(header->c_magic)) !=
        0) {
      uart_sendline("Invalid CPIO header magic number.\n");
      return -1;
    }
    unsigned long filesize = cpio_hexstr_2_ulong(header->c_filesize, 8);
    unsigned long namesize = cpio_hexstr_2_ulong(header->c_namesize, 8);
    char *pathname = (char *)(header + 1);
    char *filedata = pathname + namesize;
    filedata += ((4 - ((unsigned long)filedata & 3)) & 3);
    if (strncmp(pathname, CPIO_NEWC_TRAILER, strlen(CPIO_NEWC_TRAILER)) == 0) {
      break;
    }
    if (filename == NULL) {
      uart_sendline("%s\n", pathname);
    } else if (strcmp(pathname, filename) == 0) {
      if (cpio_is_directory(header->c_mode)) {
        uart_sendline("Error: 'cat' cannot be used to display directory.\n");
      } else {
        uart_sendline("%s\n", filedata);
      }
      return 0;
    }
    header = (cpio_newc_header_t *)(filedata + filesize);
    header = (cpio_newc_header_t *)((unsigned long)header +
                                    ((4 - ((unsigned long)header & 3)) & 3));
  }
  return filename == NULL ? 0 : -1;
}

char *cpio_extract_file_address(const char *filename) {
  cpio_newc_header_t *header = cpio_header;
  while (header != NULL) {
    if (strncmp(header->c_magic, CPIO_NEWC_MAGIC, sizeof(header->c_magic)) !=
        0) {
      uart_sendline("Invalid CPIO header magic number.\n");
      return NULL;
    }
    unsigned long filesize = cpio_hexstr_2_ulong(header->c_filesize, 8);
    unsigned long namesize = cpio_hexstr_2_ulong(header->c_namesize, 8);
    char *pathname = (char *)(header + 1);
    char *filedata = pathname + namesize;
    filedata += ((4 - ((unsigned long)filedata & 3)) & 3);
    if (strncmp(pathname, CPIO_NEWC_TRAILER, strlen(CPIO_NEWC_TRAILER)) == 0) {
      break;
    }
    if (strcmp(pathname, filename) == 0) {
      return filedata;
    }
    header = (cpio_newc_header_t *)(filedata + filesize);
    header = (cpio_newc_header_t *)((unsigned long)header +
                                    ((4 - ((unsigned long)header & 3)) & 3));
  }
  return NULL;
}