#ifndef _VFS_H
#define _VFS_H

#include "types.h"

#define MAX_PATHNAME_LEN 255
#define MAX_SUBDIR_N 32
#define MAX_FD 16

#define REGULAR_FILE 0
#define DIRECTORY 1
#define DEVICE 2

#define O_CREAT 0b01000000

#define EOF -1

typedef struct vnode vnode;
typedef struct mount mount;
typedef struct filesystem filesystem;
typedef struct file file;
typedef struct vnode_operations vnode_operations;
typedef struct file_operations file_operations;
typedef int32_t (*vfs_create_fp_t)(vnode*, vnode**, const char*);
typedef int32_t (*vfs_lookup_fp_t)(vnode*, vnode**, const char*);
typedef int32_t (*vfs_mkdir_fp_t)(vnode*, vnode** target, const char*);
typedef int32_t (*vfs_open_fp_t)(vnode*, file**);
typedef int32_t (*vfs_close_fp_t)(file*);
typedef int32_t (*vfs_read_fp_t)(file*, void*, uint32_t);
typedef int32_t (*vfs_write_fp_t)(file*, const void*, uint32_t);
typedef int32_t (*vfs_setup_mount)(filesystem*, mount*, const char*);
typedef int32_t (*vfs_register_fs)();
typedef int32_t (*vfs_syncfs)();

struct mount {
  vnode* root;  // root directory
  filesystem* fs;
};

struct vnode {
  char name[16];
  vnode *parent, *subdirs[MAX_SUBDIR_N];
  mount* mountpoint;
  vnode_operations* v_ops;
  file_operations* f_ops;
  void* internal;
  uint8_t type;
};

struct filesystem {
  char name[16];
  vfs_register_fs register_fs;
  vfs_setup_mount setup_mnt;
  vfs_syncfs syncfs;
};

// file handle
struct file {
  vnode* vnode;
  file_operations* f_ops;
  uint32_t f_pos;  // RW position of this file handle
  uint8_t flags;
};

struct file_operations {
  vfs_write_fp_t write;
  vfs_read_fp_t read;
  vfs_open_fp_t open;
  vfs_close_fp_t close;
};

struct vnode_operations {
  vfs_lookup_fp_t lookup;
  vfs_create_fp_t create;
  vfs_mkdir_fp_t mkdir;
};

int32_t register_filesystem(struct filesystem* fs);
file* vfs_open(const char* pathname, uint8_t flags);
int32_t vfs_close(file* fp);
int32_t vfs_write(file* fp, const void* buf, uint32_t len);
int32_t vfs_read(file* fp, void* buf, uint32_t len);
int32_t vfs_mkdir(const char* pathname);
int32_t vfs_mount(const char* mountpoint, const char* fs);
int32_t vfs_chdir(const char* pathname);
int32_t vfs_lseek64(file* fp, int64_t offset, int32_t whence);
int32_t vfs_syncall();

void rootfs_init();
#endif