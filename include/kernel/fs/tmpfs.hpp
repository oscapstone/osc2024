#pragma once

#include "fs/vfs.hpp"
#include "string.hpp"

namespace tmpfs {

::FileSystem* init();

class Vnode final : public ::Vnode {
 public:
  string content;

  using ::Vnode::Vnode;
  virtual ~Vnode() = default;
  virtual int create(const char* component_name, ::Vnode*& vnode);
  virtual int mkdir(const char* component_name, ::Vnode*& vnode);
  virtual int open(::File*& file, fcntl flags);
};

class File final : public ::File {
  Vnode* get() const {
    // XXX: no rtii
    return static_cast<Vnode*>(vnode);
  }

  using ::File::File;
  virtual ~File() = default;
  virtual long size() const;
  virtual bool can_seek() const;

  virtual int write(const void* buf, size_t len);
  virtual int read(void* buf, size_t len);
};

class FileSystem final : public ::FileSystem {
 public:
  FileSystem();

  virtual ::Vnode* mount(const char* name);
};

}  // namespace tmpfs
