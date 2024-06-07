#pragma once

#include "fs/ds.hpp"
#include "fs/ds_impl.hpp"

namespace fat32fs {

class Vnode;
class File;
class FileSystem;

class Vnode final : public ::VnodeImpl<Vnode, File> {
  int _filesize;

 public:
  using ::VnodeImpl<Vnode, File>::VnodeImpl;
  virtual ~Vnode() = default;
  virtual long filesize() const {
    return _filesize;
  }
};

class File final : public ::FileImplRW<Vnode, File> {
  virtual bool resize(size_t new_size) {
    return false;
  }
  virtual char* write_ptr() {
    return nullptr;
  }
  virtual char* read_ptr() {
    return nullptr;
  }

 public:
  using ::FileImplRW<Vnode, File>::FileImplRW;
  virtual ~File() = default;
};

class FileSystem final : public ::FileSystem {
  static ::Vnode* root;
  static bool init;

 public:
  FileSystem();

  virtual const char* name() const {
    return "fat32fs";
  }

  virtual ::Vnode* mount() {
    return root;
  }
};

}  // namespace fat32fs
