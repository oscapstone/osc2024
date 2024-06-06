#pragma once

#include "fs/cpio.hpp"
#include "fs/ds.hpp"
#include "fs/ds_impl.hpp"

namespace initramfs {

extern CPIO cpio;

inline void* startp() {
  return cpio.startp();
}
inline void* endp() {
  return cpio.endp();
}

class Vnode;
class File;
class FileSystem;

void preinit();

class Vnode final : public ::VnodeImpl<Vnode, File> {
  friend File;
  const char* _content;
  int _size;

 public:
  Vnode(const cpio_newc_header* hdr)
      : ::VnodeImpl<Vnode, File>{hdr->isdir() ? kDir : kFile},
        _content{hdr->file_ptr()},
        _size{hdr->filesize()} {}
  virtual ~Vnode() = default;
  long size() const {
    return _size;
  }
};

class File final : public ::FileImplRW<Vnode, File> {
  virtual const char* read_ptr() {
    return this->get()->_content;
  }

 public:
  using ::FileImplRW<Vnode, File>::FileImplRW;
  virtual ~File() = default;
};

class FileSystem final : public ::FileSystem {
 public:
  ::Vnode* root;

  FileSystem();
  virtual const char* name() {
    return "initramfs";
  }

  virtual ::Vnode* mount() {
    return root;
  }
};

};  // namespace initramfs
