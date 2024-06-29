#include "initramfs.h"

#include <stdint.h>

#include "string.h"
#include "uart.h"
#include "utils.h"

static void *ramfs_base = (void *)0x0000000;

void initramfs_callback(void *addr) {
  uart_puts("[INFO] Initial Ramdisk is mounted at ");
  uart_hex((uintptr_t)addr);
  uart_putc('\n');
  ramfs_base = (char *)addr;
}

void initramfs_list() {
  char *fptr = (char *)ramfs_base;

  // Check if the file is encoded with New ASCII Format
  while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
    cpio_t *header = (cpio_t *)fptr;

    // New ASCII Format uses 8-byte hexadecimal string for all numbers
    int namesize = hextoi(header->c_namesize, 8);
    int filesize = hextoi(header->c_filesize, 8);

    // Total size of (header + pathname) is a multiple of four bytes
    // File data is also padded to a multiple of four bytes
    int headsize = align4(sizeof(cpio_t) + namesize);
    int datasize = align4(filesize);

    // Print file pathname
    char pathname[namesize];
    strncpy(pathname, fptr + sizeof(cpio_t), namesize);
    uart_puts(pathname);
    uart_putc('\n');

    fptr += headsize + datasize;
  }
}

void initramfs_cat(const char *target) {
  char *fptr = (char *)ramfs_base;

  // Check if the file is encoded with New ASCII Format
  while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
    cpio_t *header = (cpio_t *)fptr;

    // New ASCII Format uses 8-byte hexadecimal string for all numbers
    int namesize = hextoi(header->c_namesize, 8);
    int filesize = hextoi(header->c_filesize, 8);

    // Total size of (header + pathname) is a multiple of four bytes
    // File data is also padded to a multiple of four bytes
    int headsize = align4(sizeof(cpio_t) + namesize);
    int datasize = align4(filesize);

    // Match target file
    char pathname[namesize];
    strncpy(pathname, fptr + sizeof(cpio_t), namesize);
    if (!strcmp(target, pathname)) {
      // Print its content
      char data[filesize + 1];
      strncpy(data, fptr + headsize, filesize);
      data[filesize] = '\0';
      uart_puts(data);
      uart_putc('\n');
      return;
    }

    fptr += headsize + datasize;
  }

  uart_puts("File not found.\n");
}
