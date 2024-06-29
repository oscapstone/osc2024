#include "initramfs.h"

#include <stdint.h>

#include "irq.h"
#include "mem.h"
#include "scheduler.h"
#include "start.h"
#include "str.h"
#include "uart.h"
#include "utils.h"
#include "virtm.h"

void *initrd_start;
void *initrd_end;

void initramfs_callback(void *addr, char *property) {
  if (!strcmp(property, "linux,initrd-start")) {
    uart_log(INFO, "linux,initrd-start: ");
    initrd_start = (char *)TO_VIRT(addr);
    uart_hex((uintptr_t)initrd_start);
    uart_putc(NEWLINE);
  } else if (!strcmp(property, "linux,initrd-end")) {
    uart_log(INFO, "linux,initrd-end: ");
    initrd_end = (char *)TO_VIRT(addr);
    uart_hex((uintptr_t)initrd_end);
    uart_putc(NEWLINE);
  }
}

static ramfsRec *ramfsNext(char *fptr) {
  ramfsRec *rec = kmalloc(sizeof(ramfsRec), 1);
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
    for (int i = 0; i < 15 - strlen(rec->pathname); i++) {
      uart_putc(' ');
    }
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

    if (!strcmp(filename, rec->pathname)) {
      // Load the user program
      disable_interrupt();

      thread_struct *thread = kcreate_thread(0);

      thread->user_stack = kmalloc(STACK_SIZE, 0);
      uart_log(INFO, "Acquired thread user stack: ");
      uart_hex((uintptr_t)thread->stack);
      uart_putc(NEWLINE);

      void *program = kmalloc(rec->filesize, 0);
      uart_log(INFO, "Acquired program space: ");
      uart_hex((uintptr_t)(thread->start = program));
      uart_putc(NEWLINE);
      memcpy(program, fptr + rec->headsize, thread->size = rec->filesize);

      mapping_user_thread(thread, 0);

      switch_mm((uintptr_t)TO_PHYS(thread->pgd));

      kfree(rec->pathname, 1);
      kfree(rec, 1);

      asm volatile(
          "msr tpidr_el1, %0\n"
          "msr spsr_el1, %1\n"
          "msr elr_el1, %2\n"
          "msr sp_el0, %3\n"
          "mov sp, %4\n"
          "eret\n"
          :
          : "r"(thread),                     // 0 thread_struct
            "r"(0x340),                      // 1
            "r"(0x0),                        // 2 link register
            "r"(USER_STACK + STACK_SIZE),    // 3
            "r"(thread->stack + STACK_SIZE)  // 4
          :);
      return;
    }

    fptr += rec->headsize + rec->datasize;
    kfree(rec->pathname, 1);
    kfree(rec, 1);
  }

  uart_puts("File not found.");
  uart_putc(NEWLINE);
}

// using current thread to run the loaded program
void initramfs_sys_exec(const char *target, trap_frame *tf) {
  char *fptr = (char *)initrd_start;

  while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
    ramfsRec *rec = ramfsNext(fptr);
    if (!strcmp(target, rec->pathname)) {
      thread_struct *thread = get_current();

      void *program = kmalloc(rec->filesize, 0);
      uart_log(INFO, "Acquired program space: ");
      uart_hex((uintptr_t)(thread->start = program));
      uart_putc(NEWLINE);
      memcpy(program, fptr + rec->headsize, thread->size = rec->filesize);

      memset(thread->pgd, 0, PAGE_SIZE);
      mapping_user_thread(thread, 0);

      // while (thread->sig_busy);  // wait for signal handling
      thread->sig_reg = 0;
      memset(thread->sig_handlers, 0, sizeof(thread->sig_handlers));

      kfree(rec->pathname, 1);
      kfree(rec, 1);
      return;
    }
    fptr += rec->headsize + rec->datasize;
    kfree(rec->pathname, 1);
    kfree(rec, 1);
  }
  uart_puts("File not found.\n");
}