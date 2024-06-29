#include "vfs.h"

#include "initramfs.h"
#include "mbox.h"
#include "mem.h"
#include "scheduler.h"
#include "str.h"
#include "tmpfs.h"
#include "uart.h"
#include "utils.h"
#include "vfs.h"
#include "virtm.h"

extern file_operations dev_file_operations;

mount *rootfs;
filesystem *reg_fs = 0;

void init_vfs() {
  register_filesystem("tmpfs", tmpfs_setup_mount);
  rootfs = kmalloc(sizeof(mount), SILENT);
  reg_fs->setup_mount(reg_fs, rootfs);

  vfs_mkdir("/initramfs");
  register_filesystem("initramfs", initramfs_setup_mount);
  vfs_mount("/initramfs", "initramfs");

  vfs_mkdir("/dev");
  vfs_mknod("/dev/uart")->f_ops = init_dev_uart();
  vfs_mknod("/dev/framebuffer")->f_ops = init_dev_framebuffer();
}

vnode *vfs_mknod(char *pathname) {
  uart_log(VFS, "Making node: ");
  uart_puts(pathname);
  uart_putc(NEWLINE);
  file *f = create_file();
  vfs_open(pathname, O_CREAT, f);
  vnode *node = f->vnode;
  vfs_close(f);
  return node;
}

filesystem *register_filesystem(char *name, void *setup_mount) {
  // register the file system to the kernel.
  // you can also initialize memory pool of the file system here.
  uart_log(VFS, "Registering filesystem: ");
  uart_puts(name);
  uart_putc(NEWLINE);
  filesystem *fs = kmalloc(sizeof(filesystem), VERBAL);
  fs->name = kmalloc(strlen(name) + 1, SILENT);
  strncpy(fs->name, name, strlen(name) + 1);
  fs->setup_mount = setup_mount;
  fs->next = reg_fs;
  reg_fs = fs;

  filesystem *tmp = reg_fs;
  while (tmp) {
    tmp = tmp->next;
  }
  return fs;
}

int vfs_open(const char *pathname, int flags, file *target) {
  // 1. Lookup pathname
  // 2. Create a new file handle for this vnode if found.
  // 3. Create a new file if O_CREAT is specified in flags and vnode not found
  // lookup error code shows if file exist or not or other error occurs
  // 4. Return error code if fails
  int path_len = strlen(pathname);

  vnode *node;
  if (vfs_lookup(pathname, &node)) { // file not found
    if (flags & O_CREAT) {
      int last_slash_idx = 0;
      for (int i = 0; i < strlen(pathname); i++) {
        if (pathname[i] == '/')
          last_slash_idx = i;
      }

      char dirname[path_len + 1];
      strncpy(dirname, pathname, strlen(pathname) + 1);
      dirname[last_slash_idx] = 0;

      if (vfs_lookup(dirname, &node)) {
        uart_log(VFS, "Directory does not exist: ");
        uart_puts(dirname);
        uart_putc(NEWLINE);
        // kfree(dirname, SILENT);
        return -1;
      }
      node->v_ops->create(node, &node, pathname + last_slash_idx + 1);
    } else {
      uart_log(VFS, "File does not exist: ");
      uart_puts(pathname);
      uart_putc(NEWLINE);
      return -1;
    }
  }
  node = (vnode *)TO_VIRT(node);
  node->f_ops->open(node, target);
  target->flags = flags;
  return 0;
}

int vfs_close(file *file) {
  // 1. release the file handle
  // 2. Return error code if fails
  uart_log(VFS, "vfs_close: ");
  uart_dec(file->id);
  uart_putc(NEWLINE);
  return file->f_ops->close(file);
}

int vfs_write(file *file, const void *buf, size_t len) {
  // 1. write len byte from buf to the opened file.
  // 2. return written size or error code if an error occurs.
  if (file->id > 3) {
    uart_log(VFS, "vfs_write: ");
    uart_dec(file->id);
    uart_putc(NEWLINE);
  }
  return file->f_ops->write(file, buf, len);
}

int vfs_read(file *file, void *buf, size_t len) {
  // 1. read min(len, readable size) byte to buf from the opened file.
  // 2. block if nothing to read for FIFO type
  // 2. return read size or error code if an error occurs.
  if (file->id > 3) {
    uart_log(VFS, "vfs_read: ");
    uart_dec(file->id);
    uart_putc(NEWLINE);
  }
  return file->f_ops->read(file, buf, len);
}

int vfs_mkdir(const char *pathname) {
  uart_log(VFS, "vfs_mkdir: ");
  uart_puts(pathname);
  uart_putc(NEWLINE);

  char dirname[strlen(pathname)];
  char newdirname[strlen(pathname)];
  dirname[0] = '\0';
  newdirname[0] = '\0';

  int last_slash_idx = 0;
  for (int i = 0; i < strlen(pathname); i++) {
    if (pathname[i] == '/')
      last_slash_idx = i;
  }
  memcpy(dirname, pathname, last_slash_idx);
  strncpy(newdirname, pathname + last_slash_idx + 1,
          strlen(pathname) - last_slash_idx);

  vnode *node;
  if (vfs_lookup(dirname, &node) == 0) {
    node->v_ops->mkdir(node, &node, newdirname);
    return 0;
  }

  uart_log(VFS, "Path not found: ");
  uart_puts(dirname);
  uart_putc(NEWLINE);
  return -1;
}

int vfs_mount(const char *target, const char *file_sys) {
  filesystem *fs = reg_fs;

  uart_log(VFS, "Mounting filesystem: ");
  uart_puts(file_sys);
  uart_putc(NEWLINE);

  while (!fs || strcmp(fs->name, file_sys)) {
    if (!fs) {
      uart_log(VFS, "Filesystem not found\n");
      return -1;
    }
    fs = fs->next;
  }

  uart_log(VFS, "Mounting directory: ");
  uart_puts(target);
  uart_putc(NEWLINE);

  vnode *dirnode;
  if (vfs_lookup(target, &dirnode)) {
    uart_log(VFS, "Directory not found\n");
    return -1;
  }

  dirnode->mount = kmalloc(sizeof(mount), SILENT);
  fs->setup_mount(fs, dirnode->mount);
  return 0;
}

int vfs_lookup(const char *pathname, vnode **target) {
  if (strlen(pathname) == 0) {
    *target = rootfs->root;
    return 0;
  }

  vnode *dirnode = rootfs->root;
  char component_name[strlen(pathname) + 1];
  component_name[0] = '\0';
  int c_idx = 0;
  for (int i = 1; i < strlen(pathname); i++) {
    if (pathname[i] == '/') {
      component_name[c_idx++] = '\0';
      if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name)) {
        return -1;
      }
      // if mount exists, change dirnode to the root of the mounted fs
      while (dirnode->mount)
        dirnode = dirnode->mount->root;
      c_idx = 0;
    } else
      component_name[c_idx++] = pathname[i];
  }
  component_name[c_idx++] = 0;
  if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name))
    return -1;
  while (dirnode->mount)
    dirnode = dirnode->mount->root;
  *target = dirnode;
  return 0;
}

void vfs_exec(vnode *target, trap_frame *tf) {
  thread_struct *thread = get_current();
  initramfs_inode *inode = target->internal;
  void *program = kmalloc(inode->datasize, SILENT);
  kfree(thread->start, VERBAL);
  thread->start = program;
  thread->size = inode->datasize;
  uart_log(INFO, "Acquired program space: ");
  uart_hex((uintptr_t)program);
  uart_putc(NEWLINE);
  memcpy(program, inode->data, inode->datasize);

  memset(thread->pgd, 0, PAGE_SIZE);
  mapping_user_thread(thread, 0);

  while (thread->sig_busy)
    ; // wait for signal handling
  thread->sig_reg = 0;
  memset(thread->sig_handlers, 0, sizeof(thread->sig_handlers));

  // thread->context.sp = (uintptr_t)thread->stack + STACK_SIZE;
  // thread->context.fp = (uintptr_t)thread->stack + STACK_SIZE;
  // tf->elr_el1 = 0x0;
  // tf->sp_el0 = USER_STACK + STACK_SIZE;
  return;
}

file *create_file() {
  file *new_file = kmalloc(sizeof(file), SILENT);
  new_file->vnode = 0;
  new_file->f_ops = 0;
  new_file->flags = 0;
  new_file->f_pos = 0;
  new_file->prev = 0;
  new_file->next = 0;
  return new_file;
}

char *absolute_path(const char *path, const char *cwd) {
  // relative path
  char *tmp = kmalloc(strlen(cwd) + strlen(path) + 1, SILENT);
  if (path[0] != '/') {
    strncpy(tmp, cwd, strlen(cwd) + 1);
    if (strcmp(cwd, "/")) {
      strcat(tmp, "/\0");
    }
  }
  strcat(tmp, path);
  char *ret = kmalloc(strlen(tmp), SILENT);
  *ret = '\0';
  int idx = 0;
  for (int i = 0; i < strlen(tmp); i++) {
    if (tmp[i] == '/' && tmp[i + 1] == '.') {
      if (tmp[i + 2] == '.') {
        for (int j = idx; j >= 0; j--) {
          if (ret[j] == '/') {
            ret[j] = '\0';
            idx = j;
            break;
          }
        }
        i += 2;
        continue;
      } else {
        i++;
        continue;
      }
    }
    ret[idx++] = tmp[i];
  }
  ret[idx] = '\0';
  kfree(tmp, SILENT);
  return ret;
}

int not_supported() {
  uart_log(WARN, "Not supported.\n");
  return -1;
}