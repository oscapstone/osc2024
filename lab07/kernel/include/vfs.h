#ifndef _VFS_H__
#define _VFS_H__

#include "type.h"

#define DIR_NODE  0
#define FILE_NODE 1

#define MAX_FILESYSTEM        16
#define MAX_OPEN_FILE         16
#define OFFSET_FD              3 // 0, 1, 2 are reserved for stdin, stdout, stderr
#define O_CREAT         00000100

struct vnode {
  struct vnode* parent;
  struct mount* mount;
  struct vnode_operations* v_ops;
  struct file_operations* f_ops;
  void* internal;
};

// file handle
struct file {
  struct vnode* vnode;
  size_t f_pos;  // RW position of this file handle
  struct file_operations* f_ops;
  int flags;
};

struct mount {
  struct vnode* root;
  struct filesystem* fs;
};

struct filesystem {
  char* name;
  int (*setup_mount)(struct filesystem* fs, struct mount* mount);
};

struct file_operations {
  int (*write)(struct file* file, const void* buf, size_t len);
  int (*read)(struct file* file, void* buf, size_t len);
  int (*open)(struct vnode* file_node, struct file** target);
  int (*close)(struct file* file);
  // long lseek64(struct file* file, long offset, int whence);
};

struct vnode_operations {
  int (*lookup)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  int (*create)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  int (*mkdir)(struct vnode* dir_node, struct vnode** target,
              const char* component_name);
  int (*list)(struct vnode* dir_node);
};

struct vnode* vfs_get_node(const char* pathname, struct vnode** target_dir);
int register_filesystem(struct filesystem* fs);
int vfs_open(const char* pathname, int flags, struct file** target);
int vfs_close(struct file* file);
int vfs_write(struct file* file, const void* buf, size_t len);
int vfs_read(struct file* file, void* buf, size_t len);
int vfs_mkdir(const char* pathname);
int vfs_mount(const char* target, const char* filesystem);
int vfs_lookup(const char* pathname, struct vnode** target);
int vfs_list(const char* pathname);

extern struct mount* rootfs;
extern struct filesystem global_fs[MAX_FILESYSTEM];
extern struct file_descriptor_table global_fd_table;

#endif