#include "fs/ds.hpp"

#include "mm/mm.hpp"
#include "string.hpp"

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

list<Vnode::child>::iterator Vnode::find_child(const char* name) {
  auto it = _childs.begin();
  while (it != _childs.end() and *it != name)
    ++it;
  return it;
}

int Vnode::set_child(const char* name, Vnode* vnode) {
  auto it = find_child(name);
  if (it == _childs.end())
    _childs.push_back({name, vnode});
  else
    it->node = vnode;
  return 0;
}

int Vnode::del_child(const char* name) {
  auto it = find_child(name);
  if (it == _childs.end())
    return -1;
  _childs.erase(it);
  return 0;
}

int Vnode::link(const char* name, Vnode* vnode) {
  set_child(name, vnode);
  vnode->set_parent(this);
  return 0;
}

void Vnode::set_parent(Vnode* parent) {
  _parent = parent;
  set_child("..", parent);
}

Vnode::Vnode(filetype type) : type(type) {
  if (isDir())
    set_child(".", this);
}

int Vnode::lookup(const char* component_name, Vnode*& vnode) {
  if (not isDir())
    return -1;
  if (component_name[0] == 0) {
    vnode = this;
    return 0;
  }
  for (auto& n : _childs) {
    if (n == component_name) {
      vnode = n.node;
      return 0;
    }
  }
  vnode = nullptr;
  return -1;
}

const char* Vnode::lookup(const char* component_name) {
  if (not isDir())
    return nullptr;
  if (component_name[0] == 0)
    return "";
  for (auto& n : _childs)
    if (n == component_name)
      return n.name;
  return nullptr;
}

const char* Vnode::lookup(Vnode* vnode) {
  if (not isDir())
    return nullptr;
  if (vnode == this)
    return ".";
  for (const auto& n : _childs)
    if (n.node == vnode)
      return n.name;
  return nullptr;
}

long Vnode::size() const {
  return -1;
}

int Vnode::mount(const char* component_name, Vnode* new_vnode) {
  if (not isDir())
    return -1;
  for (auto& n : _childs) {
    if (n == component_name) {
      auto old_node = n.node;
      new_vnode->_prev = old_node;
      new_vnode->set_parent(old_node->parent());
      n.node = new_vnode;
      return 0;
    }
  }
  return -1;
}

int Vnode::create(const char* /*component_name*/, Vnode*& /*vnode*/) {
  return -1;
}
int Vnode::mkdir(const char* /*component_name*/, Vnode*& /*vnode*/) {
  return -1;
}
int Vnode::open(const char* /*component_name*/, FilePtr& /*file*/,
                fcntl /*flags*/) {
  return -1;
}
int Vnode::close(FilePtr file) {
  return 0;
}

int File::write(const void* /*buf*/, size_t /*len*/) {
  return -1;
}
int File::_read(const void* data, void* buf, size_t len) {
  size_t r = size() - f_pos;
  if (r > len)
    r = len;
  memcpy(buf, (char*)data + f_pos, r);
  f_pos += r;
  return r;
}
int File::read(void* /*buf*/, size_t /*len*/) {
  return -1;
}
long File::lseek64(long offset, seek_type whence) {
  if (not can_seek)
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

Vnode* FileSystem::mount() {
  return nullptr;
}
