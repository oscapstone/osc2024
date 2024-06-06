#pragma once

#include "fs/cpio.hpp"
#include "fs/ds.hpp"

namespace initramfs {

extern CPIO cpio;

inline void* startp() {
  return cpio.startp();
}
inline void* endp() {
  return cpio.endp();
}

void preinit();
::FileSystem* init();

class Vnode final : public ::Vnode {
 public:
  const char* _content;
  int _size;

  Vnode(const cpio_newc_header* hdr);
  virtual ~Vnode() = default;
  virtual long size() const;
  virtual int open(const char* component_name, ::FilePtr& file, fcntl flags);
};

class File final : public ::File {
  Vnode* get() const {
    // XXX: no rtii
    return static_cast<Vnode*>(vnode);
  }

  using ::File::File;
  virtual ~File() = default;

  virtual int read(void* buf, size_t len);
};

class FileSystem final : public ::FileSystem {
 public:
  ::Vnode* root;
  FileSystem();

  virtual ::Vnode* mount();
};

};  // namespace initramfs
