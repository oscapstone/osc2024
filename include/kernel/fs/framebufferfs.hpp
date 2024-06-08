#pragma once

#include "fs/ds.hpp"
#include "fs/ds_impl.hpp"

namespace framebufferfs {

class Vnode;
class File;
class FileSystem;

struct framebuffer_info {
  unsigned int width;
  unsigned int height;
  unsigned int pitch;
  unsigned int isrgb;
};

struct framebuffer_data {
  unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
  char* lfb;                                /* raw frame buffer address */
  size_t buf_size;
};

class Vnode final : public ::VnodeImpl<Vnode, File> {
  friend File;
  framebuffer_data* data;

 public:
  Vnode(const ::Mount* mount_root, framebuffer_data* data)
      : ::VnodeImpl<Vnode, File>{mount_root, kFile}, data(data) {}
  virtual ~Vnode() = default;
  virtual long size() const {
    return data->buf_size;
  }
};

class File final : public ::FileImplRW<Vnode, File> {
 public:
  using ::FileImplRW<Vnode, File>::FileImplRW;
  virtual ~File() = default;

  virtual char* write_ptr() {
    return get()->data->lfb;
  }
  virtual const char* read_ptr() {
    return get()->data->lfb;
  }
  virtual int ioctl(unsigned long request, void* arg);
};

class FileSystem final : public ::FileSystem {
  framebuffer_data* data;

 public:
  FileSystem();

  virtual const char* name() const {
    return "framebufferfs";
  }

  virtual ::Vnode* mount(const ::Mount* mount_root) {
    if (not data)
      return nullptr;
    return new Vnode{mount_root, data};
  }
};

}  // namespace framebufferfs
