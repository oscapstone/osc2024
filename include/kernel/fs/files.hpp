#pragma once

#include "fs/vfs.hpp"

constexpr int MAX_OPEN_FILE = 16;

struct Files {
  Vnode* cwd;
  unsigned fd_bitmap;
  // TODO: ref File*
  File* files[MAX_OPEN_FILE];
  Files(Vnode* cwd = root_node);
  Files(const Files&) = default;
  ~Files();
  void reset();
  int alloc_fd(File* file);
  void close(int fd);
  File* get(int fd) const {
    return files[fd];
  }
  int chdir(const char* path);
};

Files* current_files();
Vnode* current_cwd();
File* fd_to_file(int fd);
int chdir(const char* path);
