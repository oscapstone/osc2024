#pragma once

#include "ds/bitmask_enum.hpp"

class Vnode;
class File;
class FileSystem;

enum filetype {
  kDir = 1,
  kFile,
};

enum class fcntl {
  O_RDONLY = 00000000,
  O_WRONLY = 00000001,
  O_RDWR = 00000002,
  O_ACCMODE = 00000003,
  O_CREAT = 00000100,

  MARK_AS_BITMASK_ENUM(O_CREAT)
};
using fcntl::O_ACCMODE;
using fcntl::O_CREAT;
using fcntl::O_RDONLY;
using fcntl::O_RDWR;
using fcntl::O_WRONLY;

enum seek_type {
  SEEK_SET = 0,
};
