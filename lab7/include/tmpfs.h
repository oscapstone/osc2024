#pragma once

#include "vfs.h"

#define MAX_DIR_ENTRY 16
// #define MAX_FILE_SIZE 4096

typedef struct tmpfs_inode {
  fsnode_type type;
  char *name;
  vnode *entry[MAX_DIR_ENTRY];
  char *data;
  size_t datasize;
} tmpfs_inode;

int tmpfs_setup_mount(filesystem *fs, mount *mnt);

vnode *tmpfs_create_vnode(fsnode_type type);

int tmpfs_write(file *file, const void *buf, size_t len);

int tmpfs_read(file *file, void *buf, size_t len);

// open a file, initailze everything
int tmpfs_open(vnode *file_node, file *target);

int tmpfs_close(file *file);

// long seek 64-bit
long tmpfs_lseek64(file *file, long offset, int whence);
long tmpfs_getsize(vnode *vn);
int tmpfs_lookup(vnode *dir_node, vnode **target, const char *component_name);
int tmpfs_create(vnode *dir_node, vnode **target, const char *component_name);
int tmpfs_mkdir(vnode *dir_node, vnode **target, const char *component_name);