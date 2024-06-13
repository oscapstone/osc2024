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
  Vnode(const ::Mount* mount_root, const cpio_newc_header* hdr)
      : ::VnodeImpl<Vnode, File>{mount_root, hdr->isdir() ? kDir : kFile},
        _content{hdr->file_ptr()},
        _size{hdr->filesize()} {}
  virtual ~Vnode() = default;
  virtual long filesize() const {
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
  virtual const char* name() const {
    return "initramfs";
  }

  virtual ::Vnode* mount(const ::Mount* mount_root);
};

};  // namespace initramfs
