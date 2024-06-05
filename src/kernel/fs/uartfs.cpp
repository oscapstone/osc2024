#include "fs/uartfs.hpp"

#include "ds/list.hpp"
#include "io.hpp"

namespace uartfs {

::FileSystem* init() {
  static FileSystem* fs = nullptr;
  if (not fs)
    fs = new FileSystem;
  return fs;
}

Vnode::Vnode() : ::Vnode{kFile} {}

long Vnode::size() const {
  return 0;
}

int Vnode::open(const char* component_name, ::File*& file, fcntl flags) {
  return _open<File>(component_name, file, flags);
}

int File::write(const void* buf, size_t len) {
  return kwrite(buf, len);
}

int File::read(void* buf, size_t len) {
  return kread(buf, len);
}

FileSystem::FileSystem() : ::FileSystem{"uartfs"} {}

::Vnode* FileSystem::mount() {
  return new Vnode;
}

}  // namespace uartfs
