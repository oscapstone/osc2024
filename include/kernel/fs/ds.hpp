#pragma once

#include "ds/list.hpp"
#include "ds/smart_ptr.hpp"
#include "fs/fwd.hpp"
#include "util.hpp"

using FilePtr = smart_ptr<File>;

class Vnode {
  struct child {
    char* name;
    Vnode* node;
    child(const char* name, Vnode* node);
    child(const child&);
    child& operator=(const child&);
    ~child();
    bool operator==(const char* name) const;
  };

 private:
  // TODO: ref cnt
  Vnode *_parent = nullptr, *_prev = nullptr;
  list<child> _childs{};

 protected:
  list<child>::iterator find_child(const char* name);
  int set_child(const char* name, Vnode* vnode);
  int del_child(const char* name);

 public:
  int link(const char* name, Vnode* vnode);

  const filetype type;

  void set_parent(Vnode* parent);
  Vnode(filetype type);

  Vnode* parent() const {
    return _parent;
  }
  const list<child>& childs() const {
    return _childs;
  }

  bool isDir() const {
    return type == kDir;
  }
  bool isFile() const {
    return type == kFile;
  }

  virtual ~Vnode() = default;
  int lookup(const char* component_name, Vnode*& vnode);
  const char* lookup(const char* component_name);
  const char* lookup(Vnode* node);
  int mount(const char* component_name, Vnode* new_vnode);
  virtual long size() const;
  virtual int create(const char* component_name, Vnode*& vnode);
  virtual int mkdir(const char* component_name, Vnode*& vnode);
  virtual int open(const char* component_name, FilePtr& file, fcntl flags);
  virtual int close(FilePtr file);
};

// file handle
class File {
 protected:
  const char* _name;
  Vnode* vnode;
  size_t f_pos = 0;  // RW position of this file handle
  const fcntl flags;

 public:
  const bool can_seek = true;

  size_t size() const {
    return vnode->size();
  }
  fcntl accessmode() const {
    return flags & O_ACCMODE;
  }
  bool canRead() const {
    return accessmode() == O_RDONLY or accessmode() == O_RDWR;
  }
  bool canWrite() const {
    return accessmode() == O_WRONLY or accessmode() == O_RDWR;
  }

  File(Vnode* vnode, fcntl flags) : vnode{vnode}, flags{flags} {}
  virtual ~File() = default;

  virtual int write(const void* buf, size_t len);
  virtual int read(void* buf, size_t len);
  virtual long lseek64(long offset, seek_type whence);
  virtual int ioctl(unsigned long request, void* arg);
  int close();
};

class FileSystem {
 public:
  FileSystem* next = nullptr;
  virtual const char* name() {
    return "";
  }
  virtual Vnode* mount();
};
