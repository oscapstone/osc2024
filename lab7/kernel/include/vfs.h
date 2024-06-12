#ifndef _VFS_H_
#define _VFS_H_

#include "stddef.h"


// SPEC basic Note #3
#define MAX_FILE_NAME 15
#define MAX_DIR_ENTRY 16
#define MAX_FILE_SIZE 4096

// SPEC basic Note #4
#define MAX_PATH_NAME 255

// SPEC basic Note #5
#define MAX_FD 16

#define O_CREAT 00000100
#define SEEK_SET 0
#define MAX_FS_REG 0x50
// #define MAX_DEV_REG 0x10

enum fsnode_type
{
    dir_t,
    file_t
};


struct vnode
{
    struct mount *mount;            // Superblock        : represents mounted fs
    struct vnode_operations *v_ops; // inode & dentry Ops: represents kernel methods for vnode
    struct file_operations *f_ops;  // file Ops          : represents process methods for opened file
    void *internal;                 // vnode itself      : directly point to fs's vnode
};

// file handle
struct file
{
    struct vnode *vnode;
    size_t f_pos; // RW position of this file handle
    struct file_operations *f_ops;
    int flags;
};

struct mount
{
    struct vnode *root;
    struct filesystem *fs;
};

struct filesystem
{
    const char *name;
    int (*setup_mount)(struct filesystem *fs, struct mount *mount);
};

struct file_operations
{
    int (*write)(struct file *file, const void *buf, size_t len);
    int (*read)(struct file *file, void *buf, size_t len);
    int (*open)(struct vnode *file_node, struct file **target);
    int (*close)(struct file *file);
    long (*lseek64)(struct file *file, long offset, int whence);
    // long (*getsize)(struct vnode *vd);
};

struct vnode_operations
{
    int (*lookup)(struct vnode *dir_node, struct vnode **target,
                  const char *component_name);
    int (*create)(struct vnode *dir_node, struct vnode **target,
                  const char *component_name);
    int (*mkdir)(struct vnode *dir_node, struct vnode **target,
                 const char *component_name);
};

int register_filesystem(struct filesystem *fs);
struct filesystem *find_filesystem(const char *fs_name);
int vfs_open(const char *pathname, int flags, struct file **target);
int vfs_close(struct file *file);
int vfs_write(struct file *file, const void *buf, size_t len);
int vfs_read(struct file *file, void *buf, size_t len);
int vfs_mkdir(const char *pathname);
int vfs_mount(const char *target, const char *filesystem);
int vfs_lookup(const char *pathname, struct vnode **target);

void init_rootfs();

#endif /* _VFS_H_ */
