#pragma once

#include "fs/ds.hpp"

namespace uartfs {

::FileSystem* init();

class Vnode final : public ::Vnode {
 public:
  Vnode();
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

  virtual int write(const void* buf, size_t len);
  virtual int read(void* buf, size_t len);
};

class FileSystem final : public ::FileSystem {
 public:
  FileSystem();

  virtual ::Vnode* mount();
};

}  // namespace uartfs
