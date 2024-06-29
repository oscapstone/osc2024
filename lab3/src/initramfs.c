#include "initramfs.h"

#include <stdint.h>

#include "malloc.h"
#include "string.h"
#include "uart.h"
#include "utils.h"

static void *ramfs_base;

void initramfs_callback(void *addr, char *property) {
  if (!strcmp(property, "linux,initrd-start")) {
    uart_puts("[INFO] Initial Ramdisk is mounted at ");
    uart_hex((uintptr_t)addr);
    uart_putc(NEWLINE);
    ramfs_base = (char *)addr;
  }
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
    uart_putc(NEWLINE);

    fptr += headsize + datasize;
  }
}

void initramfs_cat(const char *filename) {
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

    // Match target filename
    char pathname[namesize];
    strncpy(pathname, fptr + sizeof(cpio_t), namesize);
    if (!strcmp(filename, pathname)) {
      // Print its content
      char data[filesize + 1];
      strncpy(data, fptr + headsize, filesize);
      data[filesize] = '\0';
      uart_puts(data);
      return;
    }

    fptr += headsize + datasize;
  }

  uart_puts("File not found.");
  uart_putc(NEWLINE);
}

void initramfs_run(const char *filename) {
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

    // Match target filename
    char pathname[namesize];
    strncpy(pathname, fptr + sizeof(cpio_t), namesize);
    if (!strcmp(filename, pathname)) {
      // Load the user program
      char *program = (char *)0x40000;
      for (int i = 0; i < filesize; i++) *program++ = (fptr + headsize)[i];
      unsigned long sp = (unsigned long)simple_malloc(DEFAULT_STACK_SIZE);
      asm volatile("msr spsr_el1, %0" ::"r"(0x3C0));
      asm volatile("msr elr_el1, %0" ::"r"(0x40000));
      asm volatile("msr sp_el0, %0" ::"r"(sp + DEFAULT_STACK_SIZE));
      asm volatile("eret;");  // Return to EL0 and execute
      return;
    }

    fptr += headsize + datasize;
  }

  uart_puts("File not found.");
  uart_putc(NEWLINE);
}