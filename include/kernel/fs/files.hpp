#pragma once

#include "fs/fwd.hpp"
#include "fs/vfs.hpp"

constexpr int MAX_OPEN_FILE = 16;

struct Files {
  Vnode* cwd;
  unsigned fd_bitmap;
  FilePtr files[MAX_OPEN_FILE];
  Files(Vnode* cwd = root_node);
  Files(const Files&) = default;
  ~Files();
  void reset();
  int alloc_fd(FilePtr file);
  void close(int fd);
  FilePtr get(int fd) const {
    if (fd < 0 or MAX_OPEN_FILE <= fd)
      return nullptr;
    return files[fd];
  }
  int chdir(const char* path);
};

Files* current_files();
Vnode* current_cwd();
FilePtr fd_to_file(int fd);
int chdir(const char* path);
