#include "vfs.h"

#include "cpio_.h"
#include "mbox.h"
#include "mem.h"
#include "multitask.h"
#include "string.h"
#include "tmpfs.h"
#include "uart1.h"
#include "utli.h"

mount rootfs_mount_point;

static vnode* vfs_lookup(const char* pathname, char* filename) {
  // parse the file pathname to get the names of the target file and its
  // directory
  vnode* target_dir_node;
  if (pathname[0] == '/') {
    pathname += 1;
    target_dir_node = rootfs_mount_point.root;
  } else if (pathname[0] == '.' && pathname[1] == '/') {
    pathname += 2;
    target_dir_node = current_thread->cwd;
  } else if (pathname[0] == '.' && pathname[1] == '.' && pathname[2] == '/') {
    pathname += 3;
    target_dir_node = current_thread->cwd->parent;
  } else {
    target_dir_node = current_thread->cwd;
  }
  if (!target_dir_node) {
    return (vnode*)0;
  }

  uint32_t f_idx = 0, b_idx = 0;
  while (pathname[b_idx] != '\0') {
    if (pathname[b_idx] == '/') {
      strncpy(filename, pathname + f_idx, b_idx - f_idx);
      if (target_dir_node->v_ops->lookup(target_dir_node, &target_dir_node,
                                         filename) == -1) {
        return (vnode*)0;  // file cannot be found
      }
      f_idx = b_idx + 1;
    }
    b_idx++;
  }
  strcpy(filename, pathname + f_idx);
  return target_dir_node;
}

static void vfs_freevnode(vnode* node) {
  // free the vnode and its child nodes recursively
  if (node == REGULAR_FILE) {
    free(node->internal);
  } else {
    for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
      if (node->subdirs[i]) {
        vfs_freevnode(node->subdirs[i]);
      }
    }
  }
  free((void*)node);
}

int32_t register_filesystem(filesystem* fs) {
  // register the file system to the kernel.
  // you can also initialize memory pool of the file system here.
  return fs->register_fs();
}

file* vfs_open(const char* pathname, uint8_t flags) {
  // 1. Lookup pathname
  // 2. Create a new file handle for this vnode if found.
  // 3. Create a new file if O_CREAT is specified in flags and vnode not found
  // lookup error code shows if file exist or not or other error occurs
  // 4. Return error code if fails
  char filename[MAX_PATHNAME_LEN];
  vnode* dir_node = vfs_lookup(pathname, filename);

  if (!dir_node) {
    uart_puts("vfs_open: dir vnode unfounded");
    return (file*)0;
  }

  file* fp;
  vnode* file_node;
  if (dir_node->v_ops->lookup(dir_node, &file_node, filename) == -1) {
    if (flags & O_CREAT) {
      if (dir_node->v_ops->create(dir_node, &file_node, filename) == -1) {
        uart_puts("vfs_open: creating file vnode fails");
        fp = (file*)0;
      } else {
        if (dir_node->f_ops->open(file_node, &fp) == -1) {
          uart_puts("vfs_open: open file fails");
          fp = (file*)0;
        }
      }
    } else {
      uart_puts("vfs_open: file vnode unfounded");
      fp = (file*)0;
    }
  } else {
    if (dir_node->f_ops->open(file_node, &fp) == -1) {
      uart_puts("vfs_open: open file fails");
      fp = (file*)0;
    }
  }
  return fp;
}

int32_t vfs_close(file* fp) {
  // 1. release the file handle
  // 2. Return error code if fails
  if (!fp) {
    return -1;
  }
  return fp->f_ops->close(fp);
}

int32_t vfs_write(file* fp, const void* buf, uint32_t len) {
  // 1. write len byte from buf to the opened file.
  // 2. return written size or error code if an error occurs.
  if (fp->vnode->type == DIRECTORY) {
    uart_puts("vfs_write error: write to a directory");
    return -1;
  }
  return fp->f_ops->write(fp, buf, len);
}

int32_t vfs_read(file* fp, void* buf, uint32_t len) {
  // 1. read min(len, readable size) byte to buf from the opened file.
  // 2. block if nothing to read for FIFO type
  // 2. return read size or error code if an error occurs.
  if (fp->vnode->type == DIRECTORY) {
    uart_puts("vfs_read error: read from a directory");
    return -1;
  }
  return fp->f_ops->read(fp, buf, len);
}

int32_t vfs_mkdir(const char* pathname) {
  char filename[MAX_PATHNAME_LEN];
  vnode* dir_node = vfs_lookup(pathname, filename);
  if (!dir_node) {
    uart_puts("vfs_mkdir: dir vnode unfounded");
    return -1;
  }
  vnode* child_dir_node;
  return dir_node->v_ops->mkdir(dir_node, &child_dir_node, filename);
}

int32_t vfs_mount(const char* mountpoint, const char* fs) {
  // mount the filesystem on the target directory
  char subdir_name[MAX_PATHNAME_LEN];
  vnode* dir_node = vfs_lookup(mountpoint, subdir_name);
  if (!dir_node) {
    uart_puts("vfs_mount: dir vnode unfounded");
    return -1;
  }
  vnode* subdir_node;
  if (dir_node->v_ops->lookup(dir_node, &subdir_node, subdir_name) == -1) {
    return -1;
  }

  filesystem* fs_t;
  if (!strcmp(fs, "tmpfs")) {
    fs_t = tmpfs_init();
  } else if (!strcmp(fs, "initramfs")) {
    fs_t = initramfs_init();
  } else if (!strcmp(fs, "uartfs")) {
    fs_t = uartfs_init();
  } else if (!strcmp(fs, "fbfs")) {
    fs_t = fbfs_init();
  } else {
    uart_puts("vfs_mount: unsupported filesystem type");
    return -1;
  }

  mount* new_mnt_p = (mount*)malloc(sizeof(mount));
  fs_t->setup_mnt(fs_t, new_mnt_p, subdir_name);
  new_mnt_p->root->parent = dir_node;

  for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
    if (dir_node->subdirs[i] &&
        !strcmp(dir_node->subdirs[i]->name, subdir_name)) {
      vfs_freevnode(dir_node->subdirs[i]);
      dir_node->subdirs[i] = new_mnt_p->root;
      break;
    }
  }
  return 0;
}

int32_t vfs_chdir(const char* pathname) {
  char subdirname[MAX_PATHNAME_LEN];
  vnode* dir_node = vfs_lookup(pathname, subdirname);

  if (!dir_node) {
    uart_puts("vfs_chdir: dir vnode unfounded");
    return -1;
  }

  vnode* subdir_node;
  if (strlen(subdirname) > 0) {
    if (dir_node->v_ops->lookup(dir_node, &subdir_node, subdirname) == -1) {
      return -1;
    }
    if (subdir_node->type !=
        DIRECTORY) {  // the type of the founded node should be the directory
      return -1;
    }
  } else {
    subdir_node = dir_node;
  }

  current_thread->cwd = subdir_node;
  return 0;
}

int32_t vfs_lseek64(file* fp, int64_t offset, int32_t whence) {
  fp->f_pos = offset;
  return 0;
}

void rootfs_init() {
  // initialize the root file system
  filesystem* tmpfs = tmpfs_init();
  register_filesystem(tmpfs);
  tmpfs->setup_mnt(tmpfs, &rootfs_mount_point, "/");

  // initialize the initram file system
  filesystem* initramfs = initramfs_init();
  register_filesystem(initramfs);
  vfs_mkdir("/initramfs");
  vfs_mount("/initramfs", "initramfs");

  // initialize the uart filesystem
  filesystem* uartfs = uartfs_init();
  register_filesystem(uartfs);
  vfs_mkdir("/dev");
  vfs_mkdir("/dev/uart");
  vfs_mount("/dev/uart", "uartfs");

  // initialize the framebuffer filesystem
  filesystem* fbfs = fbfs_init();
  register_filesystem(fbfs);
  vfs_mkdir("/dev/framebuffer");
  vfs_mount("/dev/framebuffer", "fbfs");
}