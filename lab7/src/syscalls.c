#include "syscalls.h"

#include "initramfs.h"
#include "irq.h"
#include "mem.h"
#include "scheduler.h"
#include "str.h"
#include "traps.h"
#include "uart.h"
#include "utils.h"
#include "virtm.h"

int sys_getpid() { return get_current()->pid; }

size_t sys_uart_read(char *buf, size_t size) {
  size_t i = 0;
  while (i < size)
    buf[i++] = uart_getc();
  return i;
}

size_t sys_uart_write(const char *buf, size_t size) {
  size_t i = 0;
  while (i < size)
    uart_putc(buf[i++]);
  return i;
}

int sys_exec(const char *name, trap_frame *tf) {
  thread_struct *thread = get_current();
  char *p = absolute_path(name, thread->cwd);
  vnode *target_file;
  vfs_lookup(p, &target_file); // assign the vnode to target
  kfree(p, SILENT);
  vfs_exec(target_file, tf);
  return 0;
}

extern void *lfb;

int sys_fork(trap_frame *tf) {
  disable_interrupt(); // prevent schedule premature fork

  thread_struct *parent = get_current();
  thread_struct *child = kcreate_thread(0);

  child->start = parent->start;
  child->size = parent->size;

  // list_fd(parent);
  file *fd = parent->file_descriptor;
  while (fd) {
    file *f = create_file();
    memcpy(f, fd, sizeof(file));
    push_file_descriptor(child, f);
    f->id = fd->id;
    fd = fd->next;
  }
  // list_fd(child);

  child->file_descriptor_index = parent->file_descriptor_index;

  child->user_stack = kmalloc(STACK_SIZE, SILENT);

  mapping_user_thread(child, 0x3000000);

  // Handling kernel stack (incl. tf from parent)
  memcpy(child->stack, parent->stack, STACK_SIZE);
  memcpy(child->user_stack, parent->user_stack, STACK_SIZE);

  // Copy signal handlers
  memcpy(child->sig_handlers, parent->sig_handlers,
         sizeof(parent->sig_handlers));

  // get child's trap frame from kstack via parent's offset
  size_t ksp_offset = (uintptr_t)tf - (uintptr_t)parent->stack;
  trap_frame *child_tf = (trap_frame *)(child->stack + ksp_offset);

  child->context.lr = (uintptr_t)child_ret_from_fork; // traps.S
  child->context.sp = (uintptr_t)child_tf;            // set child's ksp
  child->context.fp = (uintptr_t)child_tf;            // set child's ksp
  child_tf->sp_el0 = tf->sp_el0;                      // set child's user sp
  child_tf->x0 = 0; // ret value of fork() for child

  enable_interrupt();
  return child->pid;
}

void sys_exit(int status) {
  uart_log(status ? WARN : INFO, "Exiting process ...\n");
  kill_current_thread();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox) {
  unsigned int *itm = kmalloc(mbox[0], SILENT);
  memcpy(itm, mbox, mbox[0]);
  int ret = mbox_call(ch, (unsigned int *)itm);
  memcpy(mbox, itm, mbox[0]);
  kfree(itm, SILENT);
  return ret;
}

void sys_sigreturn(trap_frame *tf) {
  memcpy(tf, get_current()->sig_tf, sizeof(trap_frame));
  kfree(get_current()->sig_tf, SILENT);
  kfree(get_current()->sig_stack, SILENT);
  get_current()->sig_busy = 0;
  return;
}

int sys_open(const char *pathname, int flags) {
  thread_struct *thread = get_current();
  char *p = absolute_path(pathname, thread->cwd);
  file *f = create_file();
  if (vfs_open(p, flags, f))
    return -1;
  // list_fd(thread);
  file *existing = get_file_descriptor_by_vnode(thread, f->vnode);
  // uart_log(DEBUG, "existing: ");
  // uart_hex((uintptr_t)existing);
  // uart_putc(NEWLINE);
  int id = (flags & O_CREAT) || !existing ? push_file_descriptor(thread, f)
                                          : existing->id;
  uart_log(VFS, "open: ");
  uart_puts(p);
  uart_puts(", id: ");
  uart_dec(id);
  uart_puts(", flags: ");
  uart_simple_hex(flags);
  uart_putc(NEWLINE);
  kfree(p, SILENT);
  return id;
}

int sys_close(int fd) {
  thread_struct *thread = get_current();
  file *f = get_file_descriptor_by_id(thread, fd);
  // list_fd(thread);
  if (f == 0)
    return -1;
  if (thread->file_descriptor == f)
    thread->file_descriptor = f->next;
  else {
    f->prev->next = f->next;
    if (f->next)
      f->next->prev = f->prev;
  }
  // list_fd(thread);
  return vfs_close(f);
}

int sys_write(int fd, void *buf, size_t count) {
  file *f = get_file_descriptor_by_id(get_current(), fd);
  if (f == 0)
    return -1;
  return vfs_write(f, buf, count);
}

int sys_read(int fd, void *buf, size_t count) {
  // list_fd(get_current());
  file *f = get_file_descriptor_by_id(get_current(), fd);
  if (f == 0)
    return -1;
  return vfs_read(f, buf, count);
}

int sys_mkdir(const char *pathname, unsigned mode) {
  char *p = absolute_path(pathname, get_current()->cwd);
  return vfs_mkdir(p);
}

int sys_mount(const char *target, const char *filesystem) {
  char *p = absolute_path(target, get_current()->cwd);
  int ret = vfs_mount(p, filesystem);
  return ret;
}

int sys_chdir(const char *path) {
  char *p = absolute_path(path, get_current()->cwd);
  char *cwd = get_current()->cwd;
  get_current()->cwd = p;
  kfree(cwd, SILENT);
  return 0;
}

long sys_lseek64(int fd, long offset, int whence) {
  file *f = get_file_descriptor_by_id(get_current(), fd);
  if (f == 0)
    return -1;
  return f->f_ops->lseek64(f, offset, whence);
}

extern framebuffer_info *fb_info;

// ioctl 0 will be use to get info
// there will be default value in info
// if it works with default value, you can ignore this syscall
int sys_ioctl(int fd, unsigned long request, void *info) {
  if (request == GET_INFO) {
    memcpy(info, &fb_info, sizeof(framebuffer_info));
    return 0;
  }
  return not_supported();
}
