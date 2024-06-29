#pragma once

#include <stddef.h>
#include <stdint.h>

#include "traps.h"

#define O_RDONLY 00000000
#define O_WRONLY 00000001
#define O_RDWR 00000002
#define O_ACCMODE 00000003
#define O_CREAT 00000100

#define SEEK_SET 0
#define SEEK_CUR 1

typedef enum fsnode_type {
  dir_t, //
  file_t //
} fsnode_type;

typedef struct vnode {
  struct mount *mount;
  struct vnode_operations *v_ops;
  struct file_operations *f_ops;
  void *internal;
  struct vnode *next;
} vnode;

// file handle
typedef struct _file {
  int id;
  vnode *vnode;
  size_t f_pos;
  struct file_operations *f_ops;
  int flags;
  struct _file *prev;
  struct _file *next;
} file;

typedef struct mount {
  vnode *root;
  struct filesystem *fs;
} mount;

typedef struct filesystem {
  char *name;
  int (*setup_mount)(struct filesystem *fs, struct mount *mount);
  struct filesystem *next;
} filesystem;

typedef struct file_operations {
  int (*write)(file *file, const void *buf, size_t len);
  int (*read)(file *file, void *buf, size_t len);
  int (*open)(vnode *file_node, file *target);
  int (*close)(file *file);
  long (*lseek64)(file *file, long offset, int whence);
  long (*getsize)(vnode *vd);
} file_operations;

typedef struct vnode_operations {
  int (*lookup)(vnode *dir_node, vnode **target, const char *component_name);
  int (*create)(vnode *dir_node, vnode **target, const char *component_name);
  int (*mkdir)(vnode *dir_node, vnode **target, const char *component_name);
} vnode_operations;

void init_vfs();
filesystem *register_filesystem(char *const name, void *setup_mount);
vnode *vfs_mknod(char *pathname);

int vfs_open(const char *pathname, int flags, file *target);
int vfs_close(file *file);
int vfs_write(file *file, const void *buf, size_t len);
int vfs_read(file *file, void *buf, size_t len);
int vfs_mkdir(const char *pathname);
int vfs_mount(const char *target, const char *filesystem);
int vfs_lookup(const char *pathname, vnode **target);
void vfs_exec(vnode *target, trap_frame *tf);

file *create_file();
char *absolute_path(const char *path, const char *cwd);
int not_supported();