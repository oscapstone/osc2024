#include "fs/files.hpp"

#include "thread.hpp"

Files::Files() : fd_bitmap(-1), files{} {}

int Files::alloc_fd(File* file) {
  int fd = __builtin_clz(fd_bitmap);
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

Files* current_files() {
  return &current_thread()->files;
}

File* fd_to_file(int fd) {
  return current_files()->get(fd);
}
