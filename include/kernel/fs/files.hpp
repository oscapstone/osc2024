#pragma once

#include "fs/vfs.hpp"

constexpr int MAX_OPEN_FILE = 16;

struct Files {
  unsigned fd_bitmap;
  File* files[MAX_OPEN_FILE];
  Files();
  int alloc_fd(File* file);
  void close(int fd);
  File* get(int fd) const {
    return files[fd];
  }
};

Files* current_files();
File* fd_to_file(int fd);
