#include "initramfs.h"

#include <stdint.h>

#include "irq.h"
#include "mem.h"
#include "scheduler.h"
#include "start.h"
#include "str.h"
#include "syscalls.h"
#include "uart.h"
#include "utils.h"
#include "vfs.h"
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

file_operations initramfs_file_operations = {
    (void *)not_supported, // write
    initramfs_read,        //
    initramfs_open,        //
    initramfs_close,       //
    (void *)not_supported, // lseek64
    initramfs_getsize      //
};

vnode_operations initramfs_vnode_operations = {
    initramfs_lookup,      //
    (void *)not_supported, // create
    (void *)not_supported  // mkdir
};

static vnode *initramfs_create_vnode(mount *mnt, fsnode_type type) {
  vnode *v = kmalloc(sizeof(vnode), SILENT);
  v->f_ops = &initramfs_file_operations;
  v->v_ops = &initramfs_vnode_operations;
  v->mount = mnt;

  initramfs_inode *inode = kmalloc(sizeof(initramfs_inode), SILENT);
  memset(inode, 0, sizeof(initramfs_inode));
  inode->type = type;
  inode->data = kmalloc(PAGE_SIZE, SILENT);

  v->internal = inode;
  return v;
}

static ramfsRec *ramfsNext(char *fptr) {
  ramfsRec *rec = kmalloc(sizeof(ramfsRec), SILENT);
  cpio_t *header = (cpio_t *)fptr;

  // New ASCII Format uses 8-byte hexadecimal string for all numbers
  rec->namesize = hextoi(header->c_namesize, 8);
  rec->filesize = hextoi(header->c_filesize, 8);

  // Total size of (header + pathname) is a multiple of four bytes
  // File data is also padded to a multiple of four bytes
  rec->headsize = align4(sizeof(cpio_t) + rec->namesize);
  rec->datasize = align4(rec->filesize);

  // Get file pathname
  rec->pathname = kmalloc(rec->namesize, SILENT);
  strncpy(rec->pathname, fptr + sizeof(cpio_t), rec->namesize);

  return rec;
}

int initramfs_setup_mount(filesystem *fs, mount *mnt) {
  mnt->fs = fs;
  mnt->root = initramfs_create_vnode(0, dir_t);

  initramfs_inode *ramfs_inode = mnt->root->internal;
  ramfs_inode->entry = 0;

  char *fptr = (char *)initrd_start;
  while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
    ramfsRec *rec = ramfsNext(fptr);
    vnode *file_vnode = initramfs_create_vnode(0, file_t);
    initramfs_inode *file_inode = file_vnode->internal;
    file_inode->data = fptr + rec->headsize;
    file_inode->datasize = rec->filesize;
    file_inode->name = kmalloc(strlen(rec->pathname) + 1, SILENT);
    strncpy(file_inode->name, rec->pathname, strlen(rec->pathname) + 1);

    file_vnode->next = ramfs_inode->entry;
    ramfs_inode->entry = file_vnode;

    fptr += rec->headsize + rec->datasize;
    kfree(rec, SILENT);
  }
  return 0;
}

int initramfs_read(file *file, void *buf, size_t len) {
  initramfs_inode *inode = file->vnode->internal;

  // if overflow, shrink size
  if (len + file->f_pos > inode->datasize)
    len = inode->datasize - file->f_pos;

  memcpy(buf, inode->data, len);
  file->f_pos += len;
  return len;
};

int initramfs_open(vnode *file_node, file *target) {
  target->vnode = file_node;
  target->f_ops = file_node->f_ops;
  target->f_pos = 0;
  return 0;
};

int initramfs_close(file *f) {
  kfree(f, SILENT);
  return 0;
};

long initramfs_getsize(vnode *vn) {
  initramfs_inode *file_inode = vn->internal;
  return file_inode->datasize;
};

int initramfs_lookup(vnode *dir_node, vnode **target,
                     const char *component_name) {
  initramfs_inode *dir_inode = dir_node->internal;
  vnode *ptr = dir_inode->entry;
  while (ptr) {
    initramfs_inode *inode = ptr->internal;
    if (strcmp(component_name, inode->name) == 0) {
      *target = ptr;
      return 0;
    }
    ptr = ptr->next;
  }
  return -1;
};

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
    if (rec->filesize > 1)
      uart_putc('s');
    uart_putc(NEWLINE);

    fptr += rec->headsize + rec->datasize;
    kfree(rec->pathname, SILENT);
    kfree(rec, SILENT);
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
      kfree(rec->pathname, SILENT);
      kfree(rec, SILENT);
      return;
    }
    fptr += rec->headsize + rec->datasize;
    kfree(rec->pathname, SILENT);
    kfree(rec, SILENT);
  }

  uart_puts("File not found.");
  uart_putc(NEWLINE);
}

void initramfs_run(vnode *v) {
  // Load the user program
  disable_interrupt();
  initramfs_inode *inode = v->internal;

  thread_struct *thread = kcreate_thread(0);

  thread->user_stack = kmalloc(STACK_SIZE, 0);
  uart_log(INFO, "Acquired thread user stack: ");
  uart_hex((uintptr_t)thread->stack);
  uart_putc(NEWLINE);

  void *program = kmalloc(inode->datasize, 0);
  uart_log(INFO, "Acquired program space: ");
  uart_hex((uintptr_t)(thread->start = program));
  uart_putc(NEWLINE);
  memcpy(program, inode->data, thread->size = inode->datasize);

  mapping_user_thread(thread, 0x1000000);

  file *stdin = create_file();
  vfs_open("/dev/uart", 0, stdin);
  uart_log(DEBUG, "stdin: ");
  uart_dec(push_file_descriptor(thread, stdin));
  uart_putc(NEWLINE);

  file *stdout = create_file();
  vfs_open("/dev/uart", 0, stdout);
  uart_log(DEBUG, "stdout: ");
  uart_dec(push_file_descriptor(thread, stdout));
  uart_putc(NEWLINE);

  file *stderr = create_file();
  vfs_open("/dev/uart", 0, stderr);
  uart_log(DEBUG, "stderr: ");
  uart_dec(push_file_descriptor(thread, stderr));
  uart_putc(NEWLINE);

  file *framebuffer = create_file();
  vfs_open("/dev/framebuffer", 0, framebuffer);
  uart_log(DEBUG, "framebuffer: ");
  uart_dec(push_file_descriptor(thread, framebuffer));
  uart_putc(NEWLINE);

  switch_mm((uintptr_t)TO_PHYS(thread->pgd));

  // asm volatile("msr tpidr_el1, %0\n" ::"r"(thread));
  asm volatile("msr tpidr_el1, %4\n"
               "msr spsr_el1, %0\n"
               "msr elr_el1, %1\n"
               "msr sp_el0, %2\n"
               "mov sp, %3\n"
               "eret\n"
               :
               : "r"(0x340),                      // 0
                 "r"(0x0),                        // 1 link register
                 "r"(USER_STACK + STACK_SIZE),    // 2
                 "r"(thread->stack + STACK_SIZE), // 3
                 "r"(thread)                      // 4
               :);
  return;
}
