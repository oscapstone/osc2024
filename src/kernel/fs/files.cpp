#include "fs/files.hpp"

#include "fs/log.hpp"
#include "syscall.hpp"
#include "thread.hpp"

#define FS_TYPE "files"

Files::Files() : cwd(root_node), fd_bitmap(-1), files{} {}
Files::~Files() {
  reset();
}

void Files::reset() {
  for (int i = 0; i < MAX_OPEN_FILE; i++)
    close(i);
}

int Files::alloc_fd(File* file) {
  int fd = __builtin_ctz(fd_bitmap);
  if (fd >= MAX_OPEN_FILE)
    return -1;
  fd_bitmap &= ~(1u << fd);
  files[fd] = file;
  return fd;
}

void Files::close(int fd) {
  if (fd >= MAX_OPEN_FILE)
    return;
  fd_bitmap |= (1u << fd);
  files[fd] = nullptr;
}

int Files::chdir(const char* path) {
  Vnode* target;
  if (int r = vfs_lookup(path, target); r < 0)
    return r;
  cwd = target;
  return 0;
}

Files* current_files() {
  return &current_thread()->files;
}

Vnode* current_cwd() {
  return current_files()->cwd;
}

File* fd_to_file(int fd) {
  return current_files()->get(fd);
}

SYSCALL_DEFINE1(chdir, const char*, path) {
  int r = chdir(path);
  FS_INFO("chdir('%s') = %d\n", path, r);
  return r;
}

int chdir(const char* path) {
  return current_files()->chdir(path);
}
