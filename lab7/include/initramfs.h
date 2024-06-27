#pragma once

#include "traps.h"
#include "vfs.h"

// Cpio Archive File Header (New ASCII Format)
typedef struct {
  char c_magic[6];
  char c_ino[8];
  char c_mode[8];
  char c_uid[8];
  char c_gid[8];
  char c_nlink[8];
  char c_mtime[8];
  char c_filesize[8];
  char c_devmajor[8];
  char c_devminor[8];
  char c_rdevmajor[8];
  char c_rdevminor[8];
  char c_namesize[8];
  char c_check[8];
} cpio_t;

typedef struct {
  int namesize;
  int filesize;
  int headsize;
  int datasize;
  char *pathname;
} ramfsRec;

typedef struct initramfs_inode {
  fsnode_type type;
  char *name;
  vnode *entry;
  char *data;
  size_t datasize;
} initramfs_inode;

int initramfs_setup_mount(filesystem *fs, mount *mnt);

void initramfs_ls();
void initramfs_cat(const char *target);
void initramfs_callback(void *addr, char *property);
void initramfs_run(vnode *v);

int initramfs_read(file *file, void *buf, size_t len);
int initramfs_open(vnode *file_node, file *target);
int initramfs_close(file *file);
long initramfs_lseek64(file *file, long offset, int whence);
long initramfs_getsize(vnode *vn);

int initramfs_lookup(vnode *dir_node, vnode **target,
                     const char *component_name);
