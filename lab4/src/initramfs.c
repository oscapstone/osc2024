#include "initramfs.h"

#include <stdint.h>

#include "mem.h"
#include "string.h"
#include "uart.h"
#include "utils.h"

void *initrd_start;
void *initrd_end;

void initramfs_callback(void *addr, char *property) {
  if (!strcmp(property, "linux,initrd-start")) {
    uart_puts("[INFO] linux,initrd-start: ");
    uart_hex((uintptr_t)addr);
    initrd_start = (char *)addr;
    uart_putc(NEWLINE);
  } else if (!strcmp(property, "linux,initrd-end")) {
    uart_puts("[INFO] linux,initrd-end: ");
    uart_hex((uintptr_t)addr);
    initrd_end = (char *)addr;
    uart_putc(NEWLINE);
  }
}

static ramfsRec *ramfsNext(char *fptr) {
  ramfsRec *rec = (ramfsRec *)kmalloc(sizeof(ramfsRec), 1);
  cpio_t *header = (cpio_t *)fptr;

  // New ASCII Format uses 8-byte hexadecimal string for all numbers
  rec->namesize = hextoi(header->c_namesize, 8);
  rec->filesize = hextoi(header->c_filesize, 8);

  // Total size of (header + pathname) is a multiple of four bytes
  // File data is also padded to a multiple of four bytes
  rec->headsize = align4(sizeof(cpio_t) + rec->namesize);
  rec->datasize = align4(rec->filesize);

  // Get file pathname
  rec->pathname = kmalloc(rec->namesize, 1);
  strncpy(rec->pathname, fptr + sizeof(cpio_t), rec->namesize);

  return rec;
}

void initramfs_ls() {
  uart_putc(NEWLINE);

  char *fptr = (char *)initrd_start;
  while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
    ramfsRec *rec = ramfsNext(fptr);
    uart_puts(rec->pathname);
    uart_putc(TAB);
    uart_dec(rec->filesize);
    uart_puts(" byte");
    if (rec->filesize > 1) uart_putc('s');
    uart_putc(NEWLINE);

    fptr += rec->headsize + rec->datasize;
    kfree(rec->pathname, 1);
    kfree(rec, 1);
  }
}

void initramfs_cat(const char *filename) {
  char *fptr = (char *)initrd_start;

  while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
    ramfsRec *rec = ramfsNext(fptr);
    if (!strcmp(filename, rec->pathname)) {
      // Dump its content
      uart_putc(NEWLINE);
      for (char *c = fptr + rec->headsize;
           c < fptr + rec->headsize + rec->filesize; c++) {
        uart_putc(*c);
      }
      uart_putc(NEWLINE);
      uart_putc(NEWLINE);
      // kfree(data, 0);
      kfree(rec->pathname, 1);
      kfree(rec, 1);
      return;
    }
    fptr += rec->headsize + rec->datasize;
    kfree(rec->pathname, 1);
    kfree(rec, 1);
  }

  uart_puts("File not found.");
  uart_putc(NEWLINE);
}

void initramfs_run(const char *filename) {
  char *fptr = (char *)initrd_start;

  while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
    ramfsRec *rec = ramfsNext(fptr);
    // Match target filename
    if (!strcmp(filename, rec->pathname)) {
      // Load the user program
      char *program = (char *)0x40000;
      for (int i = 0; i < rec->filesize; i++)
        *program++ = (fptr + rec->headsize)[i];
      unsigned long sp = (unsigned long)kmalloc(DEFAULT_STACK_SIZE, 0);
      asm volatile(
          "msr spsr_el1, %0;"
          "msr elr_el1, %1;"
          "msr sp_el0, %2;"
          "eret"  // Return to EL0 and execute
          :
          : "r"(0x3C0), "r"(0x40000), "r"(sp + DEFAULT_STACK_SIZE)
          : "memory");
      return;
    }

    fptr += rec->headsize + rec->datasize;
    kfree(rec->pathname, 1);
    kfree(rec, 1);
  }

  uart_puts("File not found.");
  uart_putc(NEWLINE);
}