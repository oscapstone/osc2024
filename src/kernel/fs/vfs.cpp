#include "fs/vfs.hpp"

#include "fs/path.hpp"
#include "fs/tmpfs.hpp"
#include "io.hpp"
#include "string.hpp"

Vnode* root_node = nullptr;

Vnode::child::child(const char* name, Vnode* node)
    : name{strdup(name)}, node{node} {}

Vnode::child::child(const child& o) : name{strdup(o.name)}, node{o.node} {}

Vnode::child& Vnode::child::operator=(const child& o) {
  if (name)
    kfree(name);
  name = strdup(o.name);
  node = o.node;
  return *this;
}

Vnode::child::~child() {
  kfree(name);
}

bool Vnode::child::operator==(const char* component_name) const {
  return strcmp(name, component_name) == 0;
}

int Vnode::add_child(const char* name, Vnode* vnode) {
  _childs.push_back({name, vnode});
  return 0;
}

int Vnode::del_child(const char* name) {
  for (auto it = _childs.begin(); it != _childs.end(); ++it) {
    if (*it == name) {
      _childs.erase(it);
      return 0;
    }
  }
  return -1;
}

void Vnode::set_parent(Vnode* parent) {
  if (_parent) {
    _parent->del_child(_name);
    del_child("..");
  }
  _parent = parent;
  parent->add_child(_name, this);
  add_child("..", parent);
}

Vnode::Vnode(filetype type, const char* name)
    : _name(strdup(name)), type(type) {
  if (isDir()) {
    add_child("", this);
    add_child(".", this);
  }
}

Vnode::~Vnode() {
  kfree(_name);
}

int Vnode::lookup(const char* component_name, Vnode*& vnode) {
  for (auto& n : _childs) {
    if (n == component_name) {
      vnode = n.node;
      return 0;
    }
  }
  vnode = nullptr;
  return -1;
}

int Vnode::create(const char* /*component_name*/, Vnode*& /*vnode*/) {
  return -1;
}
int Vnode::mkdir(const char* /*component_name*/, Vnode*& /*vnode*/) {
  return -1;
}
int Vnode::open(File*& /*file*/, fcntl /*flags*/) {
  return -1;
}
int Vnode::close(File* file) {
  delete file;
  return 0;
}

long File::size() const {
  return -1;
}
bool File::can_seek() const {
  return true;
}
int File::write(const void* /*buf*/, size_t /*len*/) {
  return -1;
}
int File::read(void* /*buf*/, size_t /*len*/) {
  return -1;
}
long File::lseek64(long offset, seek_type whence) {
  if (not can_seek())
    return false;
  switch (whence) {
    case SEEK_SET:
      f_pos = offset;
      break;
    default:
      return -1;
  }
  return f_pos;
}
int File::close() {
  return vnode->close(this);
}

Vnode* FileSystem::mount(const char* /*name*/) {
  return nullptr;
}

void init_vfs() {
  register_filesystem(tmpfs::init());

  vfs_mount("", "tmpfs");
}

FileSystem* filesystems = nullptr;

FileSystem** find_filesystem(const char* name) {
  auto p = &filesystems;
  for (; *p; p = &(*p)->next)
    if ((*p)->name == name)
      break;
  return p;
}

FileSystem* get_filesystem(const char* name) {
  auto p = filesystems;
  for (; p; p = p->next)
    if (p->name == name)
      break;
  return p;
}

int register_filesystem(FileSystem* fs) {
  // register the file system to the kernel.
  // you can also initialize memory pool of the file system here.

  auto p = find_filesystem(fs->name);
  if (not p)
    return -1;
  *p = fs;

  return 0;
}

int vfs_open(const char* pathname, int flags, File*& target) {
  // 1. Lookup pathname
  // 2. Create a new file handle for this vnode if found.
  // 3. Create a new file if O_CREAT is specified in flags and vnode not found
  // lookup error code shows if file exist or not or other error occurs
  // 4. Return error code if fails
  return -1;
}

int vfs_close(File*& file) {
  // 1. release the file handle
  // 2. Return error code if fails
  return -1;
}

int vfs_write(File* file, const void* buf, size_t len) {
  // 1. write len byte from buf to the opened file.
  // 2. return written size or error code if an error occurs.
  return -1;
}

int vfs_read(File* file, void* buf, size_t len) {
  // 1. read min(len, readable size) byte to buf from the opened file.
  // 2. block if nothing to read for FIFO type
  // 2. return read size or error code if an error occurs.
  return -1;
}

int vfs_mkdir(const char* pathname) {
  return -1;
}

int vfs_mount(const char* target, const char* filesystem) {
  auto p_fs = find_filesystem(filesystem);
  if (not p_fs)
    return -1;
  auto fs = *p_fs;

  Vnode** p_mnt = nullptr;

  if (strcmp(target, "") == 0)
    p_mnt = &root_node;

  if (not p_mnt)
    return -1;

  auto r = fs->mount(target);
  if (not r)
    return -1;
  *p_mnt = r;
  return 0;
}

int vfs_lookup(const char* pathname, Vnode*& target) {
  auto vnode_itr = root_node;
  for (auto component_name : Path(pathname)) {
    Vnode* next_vnode;
    auto ret = vnode_itr->lookup(component_name, next_vnode);
    if (ret)
      return ret;
    vnode_itr = next_vnode;
  }
  target = vnode_itr;
  return 0;
}
