#ifndef _VFS_H_
#define _VFS_H_

#include "stddef.h"

#define MAX_PATH_NAME 255
#define MAX_FD 16
#define O_CREAT 00000100
#define SEEK_SET 0
#define MAX_FS_REG 0x50
#define MAX_DEV_REG 0x10

enum fsnode_type
{
    dir_t,
    file_t
};


struct vnode
{
    struct mount *mount;            // Superblock        : 代表 mounted file system
    struct vnode_operations *v_ops; // inode & dentry Ops: 代表vnode的核心方法
    struct file_operations *f_ops;  // file Ops          : 表示開啟檔案的方法
    void *internal;                 // vnode itself      : 直接指向fs的vnode
};

// file handle
struct file
{
    struct vnode *vnode;
    size_t f_pos; // RW position of this file handle
    struct file_operations *f_ops;
    int flags;
};

// 表示已mount的file system
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
    long (*getsize)(struct vnode *vd);
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
int register_dev(struct file_operations* fo);
struct filesystem *find_filesystem(const char *fs_name);
int vfs_open(const char *pathname, int flags, struct file **target);
int vfs_close(struct file *file);
int vfs_write(struct file *file, const void *buf, size_t len);
int vfs_read(struct file *file, void *buf, size_t len);
int vfs_mkdir(const char *pathname);
int vfs_mount(const char *target, const char *filesystem);
int vfs_lookup(const char *pathname, struct vnode **target);
int vfs_mknod(char* pathname, int id);

void init_rootfs();
void vfs_test();
char* get_absolute_path(char* path,char* curr_working_dir);

#endif /* _VFS_H_ */
