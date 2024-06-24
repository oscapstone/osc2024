#ifndef _TMPFS_H
#define _TMPFS_H

#include "fs_vfs.h"

#define TMPFS_FILE_MAXSIZE PAGE_SIZE // define in the spec
#define TMPFS_DIR_MAXSIZE 16
#define TMPFS_NAME_MAXLEN 16

#define TMPFS_TYPE_UNDEFINE 0x0
#define TMPFS_TYPE_FILE     0x1
#define TMPFS_TYPE_DIR      0x2

// init fs
filesystem *tmpfs_init(void);

// static methods
int tmpfs_mount(filesystem *fs, mount *mnt);
int tmpfs_alloc_vnode(filesystem *fs, vnode **target);

extern filesystem static_tmpfs;

int tmpfs_lookup(vnode *dir_node, vnode **target, const char *component_name);
int tmpfs_create(vnode *dir_node, vnode **target, const char *component_name);
int tmpfs_mkdir(vnode *dir_node, vnode **target, const char *component_name);
int tmpfs_isdir(vnode *dir_node);
int tmpfs_getname(vnode *dir_node, const char **name);
int tmpfs_getsize(vnode *dir_node);

extern struct vnode_operations tmpfs_v_ops;

int tmpfs_open(vnode *file_node, file *target);
int tmpfs_close(file *target);
int tmpfs_write(file *target, const void *buf, size_t len);
int tmpfs_read(file *target, void *buf, size_t len);
long tmpfs_lseek64(file *target, long offset, int whence);
int tmpfs_ioctl(struct file *file, uint64_t request, va_list args);


extern struct file_operations tmpfs_f_ops;

typedef struct tmpfs_file {
    char *data;
    int size;
    int capacity;
} tmpfs_file;

typedef struct tmpfs_dir {
    int size;
    vnode *files[TMPFS_DIR_MAXSIZE];
} tmpfs_dir;

typedef struct tmpfs_internal {
    char name[TMPFS_NAME_MAXLEN];
    int type;
    union {
        tmpfs_file *file;
        tmpfs_dir *dir;
    };
    vnode* old_node;
} tmpfs_internal;


#endif // _TMPFS_H