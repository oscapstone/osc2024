#ifndef __INITRAMFS_H__
#define __INITRAMFS_H__

#include "vfs.h"

#define INITRAMFS_MAX_NAME_LEN 256
#define INITRAMFS_MAX_ENTRIES 64

struct initramfs_node
{
  char *name;
  int is_dir;
  unsigned int size;
  char *data;
  struct vnode *entries[INITRAMFS_MAX_ENTRIES];
};

int register_initramfs();
int initramfs_init(struct vnode *root_node);

int initramfs_write(struct file *file, const void *buf, unsigned int len);
int initramfs_read(struct file *file, void *buf, unsigned int len);
int initramfs_open(struct vnode *node, struct file **file);
int initramfs_close(struct file *file);
long initramfs_lseek64(struct file *file, long offset, int whence);

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

int initramfs_setup_mount(struct filesystem *fs, struct mount *mount);

#endif