#pragma once

#include "fs/ds.hpp"
#include "fs/ds_impl.hpp"

namespace uartfs {

class Vnode;
class File;
class FileSystem;

class Vnode final : public ::VnodeImpl<Vnode, File> {
 public:
  Vnode() : ::VnodeImpl<Vnode, File>{kFile} {}
  virtual ~Vnode() = default;
  virtual long size() const {
    return -1;
  }
};

class File final : public ::File {
 public:
  using ::File::File;
  virtual ~File() = default;

  virtual int write(const void* buf, size_t len);
  virtual int read(void* buf, size_t len);
};

class FileSystem final : public ::FileSystem {
 public:
  virtual const char* name() const {
    return "uartfs";
  }

  virtual ::Vnode* mount() {
    return new Vnode;
  }
};

}  // namespace uartfs
