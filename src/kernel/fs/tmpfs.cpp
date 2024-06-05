#include "fs/tmpfs.hpp"

#include "ds/list.hpp"
#include "io.hpp"

namespace tmpfs {

::FileSystem* init() {
  static FileSystem* fs = nullptr;
  if (not fs) {
    fs = new FileSystem;
  }
  return fs;
}

long Vnode::size() const {
  return content.size();
}

int Vnode::create(const char* component_name, ::Vnode*& vnode) {
  vnode = new Vnode{kFile, component_name};
  if (vnode == nullptr)
    return -1;
  add_child(component_name, vnode);
  vnode->set_parent(this);
  return 0;
}

int Vnode::mkdir(const char* component_name, ::Vnode*& vnode) {
  vnode = new Vnode{kDir, component_name};
  if (vnode == nullptr)
    return -1;
  add_child(component_name, vnode);
  vnode->set_parent(this);
  return 0;
}

int Vnode::open(::File*& file, fcntl flags) {
  file = new File{this, flags};
  if (file == nullptr)
    return -1;
  return 0;
}

bool File::can_seek() const {
  return true;
}

int File::write(const void* buf, size_t len) {
  auto& content = get()->content;
  if (content.size() < f_pos + len)
    content.resize(f_pos + len);
  memcpy(content.data(), buf, len);
  f_pos += len;
  return len;
}

int File::read(void* buf, size_t len) {
  auto& content = get()->content;
  int r = content.size() - f_pos;
  memcpy(buf, content.data(), r);
  f_pos += r;
  return r;
}

FileSystem::FileSystem() : ::FileSystem{"tmpfs"} {}

::Vnode* FileSystem::mount(const char* name) {
  return new Vnode{kDir, name};
}

}  // namespace tmpfs
