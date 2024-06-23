#ifndef _FS_VFS_H
#define _FS_VFS_H

#include <stddef.h>
#include <stdint.h>
#include "list.h"
#include "exception.h"

#define O_CREAT 00000100

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct vnode {
    struct mount *mount;
    struct vnode_operations *v_ops;
    struct file_operations *f_ops;
    struct vnode *parent;
    void *internal;
} vnode;

typedef struct file {
    vnode *vnode;
    unsigned f_pos;
    struct file_operations *f_ops;
    int flags;
} file;

typedef struct mount {
    struct vnode *root;
    struct filesystem *fs;
} mount;

typedef struct filesystem {
    const char *name;
    struct list_head fs_list;
    int (*mount)(struct filesystem *fs, struct mount *mount);
    int (*alloc_vnode)(struct filesystem *fs, struct vnode **target);
} filesystem;

typedef struct file_operations {
    int (*write)(file *f, const void *buf, size_t len);
    int (*read)(file *f, void *buf, size_t len);
    int (*open)(vnode *file_node, file *target);
    int (*close)(file *f);
    long (*lseek64)(file *f, long offset, int whence);
} file_operations;

typedef struct vnode_operations {
    int (*lookup)(vnode *dir_node, vnode **target, const char *component_name);
    int (*create)(vnode *dir_node, vnode **target, const char *component_name);
    int (*mkdir)(vnode *dir_node, vnode **target, const char *component_name);
    int (*isdir)(vnode *dir_node);
    int (*getname)(vnode *dir_node, const char **name);
    int (*getsize)(vnode *dir_node);

} vnode_operations;


// root mount point
extern mount *rootfs;

void vfs_init();
void vfs_init_rootfs(filesystem *fs);

int register_filesystem(filesystem *fs);
int vfs_open(const char* pathname, int flags, file* target);
int vfs_close(file *f);
int vfs_write(file *f, const void *buf, size_t len);
int vfs_read(file *f, void *buf, size_t len);
int vfs_mkdir(const char *pathname);
int vfs_mount(const char* target, const char* filesystem);
int vfs_lookup(const char *pathname, vnode **target);

void syscall_open(trapframe_t *tf, const char *pathname, int flags);
void syscall_close(trapframe_t *tf, int fd);
void syscall_write(trapframe_t *tf, int fd, const void *buf, size_t len);
void syscall_read(trapframe_t *tf, int fd, void *buf, size_t len);
void syscall_mkdir(trapframe_t *tf, const char *pathname, uint32_t mode);
void syscall_mount(
    trapframe_t *tf,
    const char *src,
    const char *target,
    const char *fs_name,
    int flags,
    const void *data
);
void syscall_chdir(trapframe_t *tf, const char *path);
void syscall_lseek64(trapframe_t *tf, int fd, long offset, int whence);

// TODO: implement ioctl
void syscall_ioctl(trapframe_t *tf, int fd, unsigned long cmd, unsigned long arg);


// static methods

int open_wrapper(const char* pathname, int flags);
int close_wrapper(int fd);
int write_wrapper(int fd, const void *buf, size_t len);
int read_wrapper(int fd, void *buf, size_t len);
int mkdir_wrapper(const char* pathname);
int mount_wrapper(const char* target, const char* fs_name);
int chdir_wrapper(const char* path);


#endif // _FS_VFS_H