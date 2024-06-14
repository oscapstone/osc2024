#ifndef _VFS_H_
#define _VFS_H_

#include "stddef.h"
#include "stdint.h"
#include "list.h"
#include "sched.h"

#define MAX_PATH_NAME 255
#define MAX_FILE_NAME 20

#define O_RDONLY 00
#define O_WRONLY 01
#define O_RDWR 02
#define O_CREAT 0100
#define O_APPEND 02000
#define O_NONBLOCK 04000
#define O_TRUNC 01000
#define O_EXCL 0200

#define SEEK_SET 0
#define MAX_FS_REG 0x50
#define MAX_DEV_REG 0x50
#define MAX_NAME_BUF 1024

#define FD_TABLE_COPY(dst, src)                                                                         \
    do                                                                                                  \
    {                                                                                                   \
        for (int i = 0; i < 16; i++)                                                                    \
        {                                                                                               \
            if (src->file_descriptors_table[i] != NULL)                                                 \
            {                                                                                           \
                dst->file_descriptors_table[i] = duplicate_file_struct(src->file_descriptors_table[i]); \
            }                                                                                           \
        }                                                                                               \
    } while (0)

typedef int (*SetupMountFunc)(struct filesystem *fs, struct mount *superblock, struct vnode *parent, const char *name);

typedef enum fsnode_type
{
    FS_DIR,
    FS_FILE,
    FS_DEV,
    FS_PIPE,
    FS_SYMLINK,
    FS_TYPE_MAX
} fsnode_type_t;

typedef struct vnode
{
    struct mount *superblock;             // Superblock        : represents mounted fs
    struct mount *mount;                  // Mount point       : represents mounted fs
    struct vnode *parent;                 // Parent directory  : represents parent directory
    const struct vnode_operations *v_ops; // inode & dentry Ops: represents kernel methods for vnode
    const struct file_operations *f_ops;  // file Ops          : represents process methods for opened file
    enum fsnode_type type;                // Type              : represents file type
    char *name;                           // Name              : represents file name
    void *internal;                       // vnode itself      : directly point to fs's vnode
} vnode_t;

typedef struct vnode_list
{
    list_head_t list_head;
    struct vnode *vnode;
} vnode_list_t;

// file handle
typedef struct file
{
    struct vnode *vnode;
    size_t f_pos; // RW position of this file handle
    const struct file_operations *f_ops;
    int flags;
} file_t;

typedef struct mount
{
    struct vnode *root;
    struct filesystem *fs;
    const struct vnode_operations *v_ops; // inode & dentry Ops: represents kernel methods for vnode
    const struct file_operations *f_ops;  // file Ops          : represents process methods for opened file
} mount_t;

typedef struct filesystem
{
    const char *name;
    SetupMountFunc setup_mount;
} filesystem_t;

typedef struct file_operations
{
    int (*write)(struct file *file, const void *buf, size_t len);
    int (*read)(struct file *file, void *buf, size_t len);
    int (*open)(struct vnode *file_node, struct file **target);
    int (*close)(struct file *file);
    long (*lseek64)(struct file *file, long offset, int whence);
    long (*getsize)(struct vnode *vd);
} file_operations_t;

typedef struct dev
{
    const char *name;
    const struct file_operations *f_ops;
} dev_t;

typedef struct vnode_operations
{
    int (*lookup)(struct vnode *dir_node, struct vnode **target, const char *component_name);
    int (*create)(struct vnode *dir_node, struct vnode **target, const char *component_name);
    int (*mkdir)(struct vnode *dir_node, struct vnode **target, const char *component_name);
    int (*readdir)(struct vnode *dir_node, const char name_array[]);
} vnode_operations_t;

int register_filesystem(struct filesystem *fs);
int handling_relative_path(const char *path, vnode_t *curr_vnode, vnode_t **target, size_t *start_idx);
vnode_t *create_vnode();
int register_dev(dev_t *dev);
int vfs_open(struct vnode *dir_node, const char *pathname, int flags, struct file **target);
int vfs_close(struct file *file);
int vfs_write(struct file *file, const void *buf, size_t len);
int vfs_read(struct file *file, void *buf, size_t len);
int vfs_mkdir(struct vnode *dir_node, const char *pathname);
int vfs_mount(struct vnode *dir_node, const char *target, const char *filesystem);
int vfs_lookup(struct vnode *dir_node, const char *pathname, struct vnode **target);
int vfs_mknod(struct vnode *dir_node, char *pathname, int id);

void init_rootfs();
void init_thread_vfs(struct thread_struct *t);
vnode_t *get_root_vnode();
int get_pwd(char *buf);
struct file *duplicate_file_struct(struct file *file);
void vfs_test();
char *get_absolute_path(char *path, char *curr_working_dir);

#endif /*_VFS_H_*/