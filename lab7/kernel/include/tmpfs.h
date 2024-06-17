#ifndef TMPFS_H
#define TMPFS_H

#include "vfs.h"

typedef struct tmpfs_node {
	char data[MAX_DATA_SIZE];
	char name[MAX_NAME_SIZE];
	vnode* entry[MAX_ENTRY_SIZE];
	int type; // 1: directory, 2: file
	int size;
} tmpfs_node;

int tmpfs_write(struct file *file, const void *buf, size_t len);
int tmpfs_read(struct file *file, void *buf, size_t len);
int tmpfs_open(struct vnode *file_node, struct file **target);
int tmpfs_close(struct file *file);
long tmpfs_lseek64(int fd, long offset, int whence);

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

int tmpfs_mount(struct filesystem *fs, struct mount *mt);

#endif
