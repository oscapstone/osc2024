#ifndef __MAILBOX_H
#define __MAILBOX_H

#include "vfs.h"

extern volatile unsigned int mailbox[36];

int framebufferfs_setup_mount(struct filesystem *fs, struct mount *mount);

int framebufferfs_write(struct file *file, const void *buf, size_t len);
int framebufferfs_read(struct file *file, void *buf, size_t len);

struct dentry *framebufferfs_lookup(struct inode *dir, const char* component_name);
int framebufferfs_create(struct inode *dir, struct dentry *dentry, int mode);
int framebufferfs_mkdir(struct inode *dir, struct dentry *dentry);

int mailbox_call(unsigned char ch);

#endif