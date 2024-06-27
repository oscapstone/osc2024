#ifndef __VFS_H__
#define __VFS_H__

#define MAX_
#define O_CREAT 64

enum SEEK_POS
{
  SEEK_SET,
  SEEK_CUR,
  SEEK_END
};

struct vnode
{
  struct vnode *parent;
  struct mount *mount;
  struct vnode_operations *v_ops;
  struct file_operations *f_ops;
  void *internal;
};

// file handle
struct file
{
  struct vnode *vnode;
  unsigned int f_pos; // RW position of this file handle
  struct file_operations *f_ops;
  int flags;
};

struct mount
{
  struct vnode *root;
  struct filesystem *fs;
};

struct filesystem
{
  const char *name;
  int (*setup_mount)(struct filesystem *fs, struct mount *mount);
};

struct file_operations
{
  int (*write)(struct file *file, const void *buf, unsigned int len);
  int (*read)(struct file *file, void *buf, unsigned int len);
  int (*open)(struct vnode *file_node, struct file **target);
  int (*close)(struct file *file);
  long (*lseek64)(struct file *file, long offset, int whence);
};

struct vnode_operations
{
  int (*lookup)(struct vnode *dir_node, struct vnode **target,
                const char *component_name);
  int (*create)(struct vnode *dir_node, struct vnode **target,
                const char *component_name);
  int (*mkdir)(struct vnode *dir_node, struct vnode **target,
               const char *component_name);
};

int register_filesystem(struct filesystem *fs);

int vfs_open(const char *pathname, int flags, struct file **target);
int vfs_close(struct file *file);
int vfs_write(struct file *file, const void *buf, unsigned int len);
int vfs_read(struct file *file, void *buf, unsigned int len);
int vfs_lseek64(struct file *file, long offset, int whence);

int vfs_mkdir(const char *pathname);
int vfs_mount(const char *target, const char *filesystem);
int vfs_lookup(const char *pathname, struct vnode **target);

int init_vfs();

#endif