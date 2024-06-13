#pragma once

#include <concepts>

#include "fs/ds.hpp"
#include "string.hpp"

template <class V, class F>
class VnodeImpl : public Vnode {
 public:
  using Vnode::Vnode;
  virtual ~VnodeImpl() = default;

  virtual int open(const char* /*component_name*/, FilePtr& file, fcntl flags) {
    file = new F{this, flags};
    if (file == nullptr)
      return -1;
    return 0;
  }
};

template <class V, class F>
class VnodeImplRW : public VnodeImpl<V, F> {
 public:
  using VnodeImpl<V, F>::VnodeImpl;
  virtual ~VnodeImplRW() = default;

  virtual int create(const char* component_name, ::Vnode*& vnode) {
    vnode = new V{this->mount_root, kFile};
    if (vnode == nullptr)
      return -1;
    this->set_child(component_name, vnode);
    vnode->set_parent(this);
    return 0;
  }

  virtual int mkdir(const char* component_name, ::Vnode*& vnode) {
    vnode = new V{this->mount_root, kDir};
    if (vnode == nullptr)
      return -1;
    this->set_child(component_name, vnode);
    vnode->set_parent(this);
    return 0;
  }
};

template <class V, class F>
class FileImpl : public File {
 public:
  V* get() const {
    // XXX: no rtii
    return static_cast<V*>(this->vnode);
  }

  using ::File::File;
  virtual ~FileImpl() = default;
};

template <class V, class F>
class FileImplRW : public FileImpl<V, F> {
  virtual bool resize(size_t /*new_size*/) {
    return false;
  }
  virtual char* write_ptr() {
    return nullptr;
  }
  virtual const char* read_ptr() {
    return nullptr;
  }

 public:
  using FileImpl<V, F>::FileImpl;
  virtual ~FileImplRW() = default;

  virtual int write(const void* buf, size_t len) {
    auto end = this->f_pos + len;
    if (this->size() < end)
      if (not resize(end))
        return -1;
    auto p = write_ptr();
    if (not p)
      return -1;
    memcpy(p + this->f_pos, buf, len);
    this->f_pos += len;
    return len;
  }

  virtual int read(void* buf, size_t len) {
    size_t r = this->size() - this->f_pos;
    if (r > len)
      r = len;
    auto p = read_ptr();
    if (not p)
      return -1;
    memcpy(buf, p + this->f_pos, r);
    this->f_pos += r;
    return r;
  };
};
