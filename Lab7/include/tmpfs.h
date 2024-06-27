#ifndef __TMPFS_H__
#define __TMPFS_H__

#include "vfs.h"

#define TMPFS_MAX_NAME_LEN 15
#define TMPFS_MAX_ENTRIES 16
#define TMPFS_MAX_FILE_SIZE 4096

struct tmpfs_node
{
  char name[TMPFS_MAX_NAME_LEN + 1];
  int is_dir;
  unsigned int size;
  char *data;
  struct vnode *entries[TMPFS_MAX_ENTRIES];
};

int register_tmpfs();
int tmpfs_write(struct file *file, const void *buf, unsigned int len);
int tmpfs_read(struct file *file, void *buf, unsigned int len);
int tmpfs_open(struct vnode *node, struct file **file);
int tmpfs_close(struct file *file);
long tmpfs_lseek64(struct file *file, long offset, int whence);

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

int tmpfs_setup_mount(struct filesystem *fs, struct mount *mount);

#endif