#pragma once

#include "fs/ds.hpp"
#include "fs/ds_impl.hpp"
#include "string.hpp"

namespace tmpfs {

class Vnode;
class File;
class FileSystem;

class Vnode final : public ::VnodeImplRW<Vnode, File> {
 public:
  string content;

  using ::VnodeImplRW<Vnode, File>::VnodeImplRW;
  virtual ~Vnode() = default;
  virtual long size() const {
    return content.size();
  }
};

class File final : public ::FileImplRW<Vnode, File> {
  auto& content() {
    return get()->content;
  }
  virtual bool resize(size_t new_size) {
    content().resize(new_size);
    return true;
  }
  virtual char* write_ptr() {
    return content().data();
  }
  virtual char* read_ptr() {
    return content().data();
  }

 public:
  using ::FileImplRW<Vnode, File>::FileImplRW;
  virtual ~File() = default;
};

class FileSystem final : public ::FileSystem {
 public:
  virtual const char* name() {
    return "tmpfs";
  }

  virtual ::Vnode* mount() {
    return new Vnode{kDir};
  }
};

}  // namespace tmpfs
