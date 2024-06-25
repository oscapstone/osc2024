#ifndef __VFS_H__
#define __VFS_H__

#include "stdlib.h"

#define NR_FILESYSTEM            (8)
#define MAX_FILE_NAME_LEN        (16)  // 15 characters + '\0'
#define MAX_PATH_LEN             (256) // 255 characters + '\0'
#define MAX_DIR_NUM              (16)
#define SEEK_SET                 (0)

#define FD_TABLE_SIZE            (16)  // max open fd number is 16

#ifndef O_CREAT
#define O_CREAT                  (100)
#endif

/* file system node(vnode, inode) type: file, directory */
enum fsnode_type {
    FSNODE_TYPE_FILE,
    FSNODE_TYPE_DIR,
};

struct vnode {
    struct mount* mount;
    struct vnode_operations* v_ops;
    struct file_operations* f_ops;
    void* internal;
};

// file handle
struct file {
    struct vnode* vnode;
    size_t f_pos;  // RW position of this file handle
    struct file_operations* f_ops;
    int flags;
};

struct mount {
    struct vnode* root;
    struct filesystem* fs;
};

struct filesystem {
    const char* name;
    int (*setup_mount)(struct filesystem* fs, struct mount* mount);
};

struct file_operations {
    int (*write)(struct file* file, const void* buf, size_t len);
    int (*read)(struct file* file, void* buf, size_t len);
    int (*open)(struct vnode* file_node, struct file** target); // Derive struct file from vnode
    int (*close)(struct file* file);
    long (*lseek64)(struct file* file, long offset, int whence);
};

struct vnode_operations {
    int (*lookup)(struct vnode* dir_node, struct vnode** target,
                  const char* component_name);
    int (*create)(struct vnode* dir_node, struct vnode** target,
                  const char* component_name);
    int (*mkdir)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
};

extern struct filesystem *filesystems[NR_FILESYSTEM];
extern struct mount* rootfs;

void rootfs_init(void);

int register_filesystem(struct filesystem *fs);
int vfs_open(const char* pathname, int flags, struct file** target);
int vfs_close(struct file* file);
int vfs_write(struct file* file, const void* buf, size_t len);
int vfs_read(struct file* file, void* buf, size_t len);

int vfs_mkdir(const char* path);
int vfs_mount(const char* target, const char* filesystem);
int vfs_lookup(const char *pathname, struct vnode **target);

/* Get absolute path from current path, store it to path. */
void get_absolute_path(char *path, char *current_path);

#endif // __VFS_H__