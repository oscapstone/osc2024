#ifndef __TMPFS_H
#define __TMPFS_H

#include "vfs.h"

struct tmpfs_internal
{
    char data[4096];
};


int tmpfs_mount(struct filesystem *fs, struct mount *mount);

int tmpfs_write(struct file *file, const void *buf, size_t len);
int tmpfs_read(struct file *file, void *buf, size_t len);
struct dentry *tmpfs_lookup(struct inode *dir, const char* component_name);
int tmpfs_create(struct inode *dir, struct dentry *dentry, int mode);

#endif