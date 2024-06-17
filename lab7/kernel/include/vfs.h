#ifndef VFS_H
#define VFS_H

#include <stddef.h>

#define O_CREAT 00000100

#define MAX_FS_NUM 100
#define MAX_PATH_SIZE 255
#define MAX_DATA_SIZE 4096
#define MAX_NAME_SIZE 15
#define MAX_ENTRY_SIZE 16

typedef struct vnode {
  struct mount* mount;
  struct vnode_operations* v_ops;
  struct file_operations* f_ops;
  void* internal;
} vnode;

// file handle
typedef struct file {
  struct vnode* vnode;
  unsigned long f_pos;  // RW position of this file handle
  struct file_operations* f_ops;
  int flags;
  int ref; // not used
} file;

typedef struct mount {
  struct vnode* root;
  struct filesystem* fs;
} mount;

typedef struct filesystem {
  char* name;
  int (*setup_mount)(struct filesystem* fs, struct mount* mount);
} filesystem;

typedef struct file_operations {
  int (*write)(struct file* file, const void* buf, size_t len);
  int (*read)(struct file* file, void* buf, size_t len);
  int (*open)(struct vnode* file_node, struct file** target);
  int (*close)(struct file* file);
  // long lseek64(struct file* file, long offset, int whence);
} file_operations;

typedef struct vnode_operations {
  int (*lookup)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  int (*create)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  int (*mkdir)(struct vnode* dir_node, struct vnode** target,
              const char* component_name);
} vnode_operations;

int register_filesystem(struct filesystem* fs);
  // register the file system to the kernel.
  // you can also initialize memory pool of the file system here.

int vfs_open(const char* pathname, int flags, struct file** target);
  // 1. Lookup pathname
  // 2. Create a new file handle for this vnode if found.
  // 3. Create a new file if O_CREAT is specified in flags and vnode not found
  // lookup error code shows if file exist or not or other error occurs
  // 4. Return error code if fails

int vfs_close(struct file* file);
  // 1. release the file handle
  // 2. Return error code if fails

int vfs_write(struct file* file, const void* buf, size_t len);
  // 1. write len byte from buf to the opened file.
  // 2. return written size or error code if an error occurs.

int vfs_read(struct file* file, void* buf, size_t len);
  // 1. read min(len, readable size) byte to buf from the opened file.
  // 2. block if nothing to read for FIFO type
  // 2. return read size or error code if an error occurs.

int vfs_mkdir(const char* pathname);
int vfs_mount(const char* target, const char* filesystem);
int vfs_lookup(const char* pathname, struct vnode** target);

void filesystem_init();

const char* to_absolute(char*, char*);

#endif
