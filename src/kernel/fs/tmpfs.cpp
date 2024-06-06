#include "fs/tmpfs.hpp"

namespace tmpfs {

::FileSystem* init() {
  static FileSystem* fs = nullptr;
  if (not fs)
    fs = new FileSystem;
  return fs;
}

}  // namespace tmpfs
