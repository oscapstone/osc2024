#include "initramfs.h"

#include <stdint.h>

#include "mem.h"
#include "scheduler.h"
#include "str.h"
#include "uart.h"
#include "utils.h"

void *initrd_start;
void *initrd_end;

void initramfs_callback(void *addr, char *property) {
  if (!strcmp(property, "linux,initrd-start")) {
    uart_log(INFO, "linux,initrd-start: ");
    uart_hex((uintptr_t)addr);
    initrd_start = (char *)addr;
    uart_putc(NEWLINE);
  } else if (!strcmp(property, "linux,initrd-end")) {
    uart_log(INFO, "linux,initrd-end: ");
    uart_hex((uintptr_t)addr);
    initrd_end = (char *)addr;
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
      void *program = kmalloc(rec->filesize, 0);
      memcpy(program, fptr + rec->headsize, rec->filesize);

      thread_struct *thread = kcreate_thread((void *)program);
      thread->user_stack = kmalloc(STACK_SIZE, 0);
      asm volatile(
          "msr tpidr_el1, %0\n"
          "msr spsr_el1, %1\n"
          "msr elr_el1, %2\n"
          "msr sp_el0, %3\n"
          "mov sp, %4\n"
          "eret\n"
          :
          : "r"(thread),                           // 0 thread_struct
            "r"(0x340),                            // 1
            "r"(thread->context.lr),               // 2 link register
            "r"(thread->user_stack + STACK_SIZE),  // 3
            "r"(thread->context.sp)                // 4
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
      void *program = kmalloc(rec->filesize, 0);
      memcpy(program, fptr + rec->headsize, rec->filesize);
      thread_struct *thread = get_current();
      while (thread->sig_busy);  // wait for signal handling
      thread->sig_reg = 0;
      memset(thread->sig_handlers, 0, sizeof(thread->sig_handlers));
      thread->context.lr = (unsigned long)program;  // change to elr
      thread->context.sp = (uintptr_t)thread->stack + STACK_SIZE;
      thread->context.fp = (uintptr_t)thread->stack + STACK_SIZE;
      tf->elr_el1 = (unsigned long)program;
      if (thread->user_stack == 0) thread->user_stack = kmalloc(STACK_SIZE, 0);
      tf->sp_el0 = (uintptr_t)thread->user_stack + STACK_SIZE;
      return;
    }
    fptr += rec->headsize + rec->datasize;
  }
  uart_puts("File not found.\n");
}