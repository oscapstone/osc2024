#ifndef VFS_H
#define VFS_H

#include "kernel/type.h"
#include "kernel/utils.h"
#include "kernel/allocator.h"

#define MAX_PATHNAME 255
#define O_CREAT 00000100
#define MAX_FILE_SIZE 4096
#define MAX_FS 0x50
#define MAX_DEV 0x10
#define MAX_FILE_NAME_LEN 15  // should for tmpfs

#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */
#define SEEK_DATA	3	/* Seek to next data.  */
#define SEEK_HOLE	4	/* Seek to next hole.  */

// dir_t = 0, file_t = 1
enum node_type{
  dir_t,
  file_t
};

struct vnode {
  struct mount* mount;              // which mounted fs(superblock), if it's a mount point, its root will pointed to a vnode and corresponding fs inode in vnode->internal
  struct vnode_operations* v_ops;   // vnode operations
  struct file_operations* f_ops;    // open file operations
  void* internal;                   // point to fs's inode
};

// file handle
struct file {
  struct vnode* vnode;              // which vnode is opened
  my_uint64_t f_pos;                // RW position of this file handle
  struct file_operations* f_ops;
  int flags;
};

struct mount {
  struct vnode* root;
  struct filesystem* fs;
};

struct filesystem {
  const char* name;
  int (*setup_mount)(struct filesystem* fs, struct mount* mount);
};

struct file_operations {
  int (*write)(struct file* file, const void* buf, my_uint64_t len);
  int (*read)(struct file* file, void* buf, my_uint64_t len);
  int (*open)(struct vnode* file_node, struct file** target);
  int (*close)(struct file* file);
  long (*lseek64)(struct file* file, long offset, int whence);
  long (*getsize)(struct vnode *vd);
};

struct vnode_operations {
  // lookup a vnode in dir_node
  int (*lookup)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  // create a vnode in dir_node
  int (*create)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  // mkdir a vnode in dir_node
  int (*mkdir)(struct vnode* dir_node, struct vnode** target,
              const char* component_name);
};
// Don't define variables in headers. Put declarations in header and definitions in one of the .c files.
// https://stackoverflow.com/questions/69908418/multiple-definition-of-first-defined-here-on-gcc-10-2-1-but-not-gcc-8-3-0
// https://stackoverflow.com/questions/17764661/multiple-definition-of-linker-error
extern struct mount* rootfs;
extern struct filesystem filesystems[MAX_FS];
extern struct file_operations reg_dev[MAX_DEV];


int register_filesystem(struct filesystem* fs);
int register_devfs(struct file_operations* f_ops);

struct filesystem* get_fs(const char* name);
int vfs_open(const char* pathname, int flags, struct file** target);
int vfs_close(struct file* file);
int vfs_write(struct file* file, const void* buf, my_uint64_t len);
int vfs_read(struct file* file, void* buf, my_uint64_t len);
// e.g. mkdir "/dev/framebuffer"
int vfs_mkdir(const char* pathname);
// e.g. mount "devfs" on "/dev"
int vfs_mount(const char* target, const char* filesystem);
int vfs_lookup(const char* pathname, struct vnode** target);
long vfs_lseek64(struct file* file, long offset, int whence);
int op_denied(void);

int vfs_mknod(char* pathname, int id);

void init_rootfs(void);

void get_absolute_path(char *path, char *cur_working_dir);

#endif