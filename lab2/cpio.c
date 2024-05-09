#include "include/cpio.h"
#include "uart.h"
#include "utils.h"

cpio_newc_header *cpio_header = NULL;

unsigned long hex_str_2_ulong(const char *hex, int length) {
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

int is_directory(const char *mode) {
  unsigned long mode_val = hex_str_2_ulong(mode, 8);
  return (mode_val & 0170000) == 0040000;
}

int parse_cpio(cpio_newc_header *header, const char *filename) {
  while (header != NULL) {
    if (strncmp(header->c_magic, CPIO_NEWC_MAGIC, sizeof(header->c_magic)) !=
        0) {
      uart_send("Invalid CPIO header magic number.\n");
      return -1;
    }

    unsigned long filesize = hex_str_2_ulong(header->c_filesize, 8);
    unsigned long namesize = hex_str_2_ulong(header->c_namesize, 8);

    char *pathname = (char *)(header + 1);
    char *filedata = pathname + namesize;
    filedata += ((4 - ((unsigned long)filedata & 3)) & 3);

    if (strncmp(pathname, CPIO_NEWC_TRAILER, strlen(CPIO_NEWC_TRAILER)) == 0) {
      break;
    }

    if (filename == NULL) {
      uart_send("%s\n", pathname);
    } else if (strcmp(pathname, filename) == 0) {
      if (is_directory(header->c_mode)) {
        uart_send("Error: 'cat' cannot be used to display directory contents "
                  "of '%s'.\n",
                  filename);
      } else {
        uart_send("%s\n", filedata);
      }
      return 0;
    }

    header = (cpio_newc_header *)(filedata + filesize);
    header = (cpio_newc_header *)((unsigned long)header +
                                  ((4 - ((unsigned long)header & 3)) & 3));
  }
  return filename == NULL ? 0 : -1;
}
