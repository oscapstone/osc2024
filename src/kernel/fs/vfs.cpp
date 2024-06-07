#include "fs/vfs.hpp"

#include "fs/fat32fs.hpp"
#include "fs/files.hpp"
#include "fs/framebufferfs.hpp"
#include "fs/initramfs.hpp"
#include "fs/log.hpp"
#include "fs/path.hpp"
#include "fs/tmpfs.hpp"
#include "fs/uartfs.hpp"
#include "string.hpp"
#include "syscall.hpp"

#define FS_TYPE "vfs"

Vnode* root_node = nullptr;

void init_vfs() {
  register_filesystem(new tmpfs::FileSystem());
  register_filesystem(new initramfs::FileSystem());
  register_filesystem(new uartfs::FileSystem());
  register_filesystem(new framebufferfs::FileSystem());
  register_filesystem(new fat32fs::FileSystem());

  root_node = get_filesystem("tmpfs")->mount();
  root_node->set_parent(root_node);

  mkdir("/initramfs");
  mount("/initramfs", "initramfs");

  mkdir("/dev");
  mkdir("/dev/uart");
  mount("/dev/uart", "uartfs");
  mkdir("/dev/framebuffer");
  mount("/dev/framebuffer", "framebufferfs");

  mkdir("/boot");
  mount("/boot", "fat32fs");
}

FileSystem* filesystems = nullptr;

FileSystem** find_filesystem(const char* name) {
  auto p = &filesystems;
  for (; *p; p = &(*p)->next)
    if (strcmp((*p)->name(), name) == 0)
      break;
  return p;
}

FileSystem* get_filesystem(const char* name) {
  auto p = filesystems;
  for (; p; p = p->next)
    if (strcmp(p->name(), name) == 0)
      break;
  return p;
}

int register_filesystem(FileSystem* fs) {
  // register the file system to the kernel.
  // you can also initialize memory pool of the file system here.

  auto p = find_filesystem(fs->name());
  int r = 0;
  if (not p) {
    r = -1;
    goto end;
  }
  *p = fs;

end:
  FS_WARN_IF(r < 0, "register fs '%s'\n", fs->name());
  return 0;
}

SYSCALL_DEFINE2(open, const char*, pathname, fcntl, flags) {
  return open(pathname, flags);
}

int open(const char* pathname, fcntl flags) {
  int r;
  FilePtr file{};
  if ((r = vfs_open(pathname, flags, file)) < 0)
    goto end;

  r = current_files()->alloc_fd(file);
  if (r < 0)
    file->close(file);

end:
  FS_WARN_IF(r < 0, "open('%s', 0o%o) -> %d\n", pathname, flags, r);
  return r;
}

int vfs_open(const char* pathname, fcntl flags, FilePtr& target) {
  // 1. Lookup pathname
  // 2. Create a new file handle for this vnode if found.
  // 3. Create a new file if O_CREAT is specified in flags and vnode not found
  // lookup error code shows if file exist or not or other error occurs
  // 4. Return error code if fails
  Vnode *dir{}, *vnode{};
  char* basename{};
  int r = 0;
  if ((r = vfs_lookup(pathname, dir, basename)) < 0) {
    goto cleanup;
  } else {
    if (not has(flags, O_CREAT)) {
      r = dir->lookup(basename, vnode);
    } else {
      r = dir->create(basename, vnode);
    }
    if (vnode->isDir())
      r = -1;
    if (r < 0)
      goto cleanup;
    // XXX: ????
    flags = O_RDWR;
  }
  if ((r = vnode->open(dir->lookup(basename), target, flags)) < 0)
    goto cleanup;
cleanup:
  kfree(basename);
  return r;
}

SYSCALL_DEFINE1(close, int, fd) {
  return close(fd);
}

int close(int fd) {
  auto file = fd_to_file(fd);
  int r;
  if (not file) {
    r = -1;
    goto end;
  }
  if ((r = vfs_close(file)) < 0)
    goto end;
  current_files()->close(fd);
end:
  FS_WARN_IF(r < 0, "close(%d) = %d\n", fd, r);
  return 0;
}

int vfs_close(FilePtr file) {
  // 1. release the file handle
  // 2. Return error code if fails
  return file->close(file);
}

SYSCALL_DEFINE3(write, int, fd, const void*, buf, unsigned long, count) {
  return write(fd, buf, count);
}

long write(int fd, const void* buf, unsigned long count) {
  auto file = fd_to_file(fd);
  int r;
  if (not file) {
    r = -1;
    goto end;
  }
  r = vfs_write(file, buf, count);
end:
  FS_WARN_IF(r < 0, "write(%d, %p, %ld) = %d\n", fd, buf, count, r);
  return r;
}

int vfs_write(FilePtr file, const void* buf, size_t len) {
  // 1. write len byte from buf to the opened file.
  // 2. return written size or error code if an error occurs.
  if (not file->canWrite())
    return -1;
  return file->write(buf, len);
}

SYSCALL_DEFINE3(read, int, fd, void*, buf, unsigned long, count) {
  return read(fd, buf, count);
}

long read(int fd, void* buf, unsigned long count) {
  auto file = fd_to_file(fd);
  int r = -1;
  if (not file) {
    r = -1;
    goto end;
  }
  r = vfs_read(file, buf, count);
end:
  FS_WARN_IF(r < 0, "read(%d, %p, %ld) = %d\n", fd, buf, count, r);
  return r;
}

int vfs_read(FilePtr file, void* buf, size_t len) {
  // 1. read min(len, readable size) byte to buf from the opened file.
  // 2. block if nothing to read for FIFO type
  // 2. return read size or error code if an error occurs.
  if (not file->canRead())
    return -1;
  return file->read(buf, len);
}

SYSCALL_DEFINE2(mkdir, const char*, pathname, unsigned, mode) {
  return mkdir(pathname);
}

int mkdir(const char* pathname) {
  int r = vfs_mkdir(pathname);
  FS_WARN_IF(r < 0, "mkdir('%s') = %d\n", pathname, r);
  return r;
}

int vfs_mkdir(const char* pathname) {
  Vnode *dir{}, *vnode{};
  char* basename{};
  int r = 0;
  if ((r = vfs_lookup(pathname, dir, basename)) < 0)
    goto end;
  if (dir->lookup(basename, vnode) >= 0) {
    r = 0;  // XXX: ignore exist error
    goto end;
  }
  r = dir->mkdir(basename, vnode);
end:
  kfree(basename);
  return r;
}

SYSCALL_DEFINE5(mount, const char*, src, const char*, target, const char*,
                filesystem, unsigned long, flags, const void*, data) {
  return mount(target, filesystem);
}

int mount(const char* target, const char* filesystem) {
  int r = vfs_mount(target, filesystem);
  FS_WARN_IF(r < 0, "mount('%s', '%s') = %d\n", target, filesystem, r);
  return r;
}

int vfs_mount(const char* target, const char* filesystem) {
  auto fs = get_filesystem(filesystem);
  if (not fs)
    return -1;

  Vnode *dir, *new_vnode;
  char* basename;
  int r;
  if ((r = vfs_lookup(target, dir, basename)) < 0)
    goto end;

  new_vnode = fs->mount();
  if (not new_vnode) {
    r = -1;
    goto cleanup;
  }

  r = dir->mount(basename, new_vnode);

cleanup:
  kfree(basename);
end:
  return r;
}

SYSCALL_DEFINE3(lseek64, int, fd, long, offset, seek_type, whence) {
  return lseek64(fd, offset, whence);
}

int lseek64(int fd, long offset, seek_type whence) {
  auto file = fd_to_file(fd);
  int r = -1;
  if (not file) {
    r = -1;
    goto end;
  }
  r = vfs_lseek64(file, offset, whence);
end:
  FS_WARN_IF(r < 0, "lseek64(%d, 0x%lx, %d) = %d\n", fd, offset, whence, r);
  return r;
}

int vfs_lseek64(FilePtr file, long offset, seek_type whence) {
  return file->lseek64(offset, whence);
}

SYSCALL_DEFINE3(ioctl, int, fd, unsigned long, request, void*, arg) {
  return ioctl(fd, request, arg);
}

int ioctl(int fd, unsigned long request, void* arg) {
  auto file = fd_to_file(fd);
  int r = -1;
  if (not file) {
    r = -1;
    goto end;
  }
  r = vfs_ioctl(file, request, arg);
end:
  FS_WARN_IF(r < 0, "ioctl(%d, %lu, %p) = %d\n", fd, request, arg, r);
  return r;
}

int vfs_ioctl(FilePtr file, unsigned long request, void* arg) {
  return file->ioctl(request, arg);
}

Vnode* vfs_lookup(const char* pathname) {
  Vnode* target;
  if (vfs_lookup(pathname, target) < 0)
    return nullptr;
  return target;
}

int vfs_lookup(const char* pathname, Vnode*& target) {
  return vfs_lookup(current_cwd(), pathname, target);
}

int vfs_lookup(Vnode* base, const char* pathname, Vnode*& target) {
  auto vnode_itr = base;
  if (pathname[0] == '/')
    vnode_itr = root_node;
  for (auto component_name : Path(pathname)) {
    Vnode* next_vnode;
    auto ret = vnode_itr->lookup(component_name, next_vnode);
    if (ret)
      return ret;
    vnode_itr = next_vnode;
  }
  target = vnode_itr;
  return 0;
}

int vfs_lookup(const char* pathname, Vnode*& target, char*& basename) {
  return vfs_lookup(current_cwd(), pathname, target, basename);
}

int vfs_lookup(Vnode* base, const char* pathname, Vnode*& target,
               char*& basename) {
  auto vnode_itr = base;
  const char* basename_ptr = "";
  auto path = Path(pathname);
  auto it = path.begin();
  if (pathname[0] == '/')
    vnode_itr = root_node;
  while (it != path.end()) {
    auto component_name = *it;
    if (++it == path.end()) {
      basename_ptr = component_name;
      break;
    }
    Vnode* next_vnode;
    auto ret = vnode_itr->lookup(component_name, next_vnode);
    if (ret < 0)
      return ret;
    vnode_itr = next_vnode;
  }
  target = vnode_itr;
  basename = strdup(basename_ptr);
  return 0;
}
