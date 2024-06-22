#ifndef VFS_H
#define VFS_H

#include <stddef.h>

#define MAX_FS   16
#define MAX_DEV  16
#define MAX_FD   16
#define PATH_MAX 255
#define SEEK_SET 0
#define O_CREAT  00000100

enum fsnode_type { FS_DIR, FS_FILE };

struct vnode {
    struct mount *mount;
    struct file_operations *f_ops;
    struct vnode_operations *v_ops;
    void *internal;
};

struct file {
    struct vnode *vnode;
    size_t f_pos;
    struct file_operations *f_ops;
    int flags;
};

struct mount {
    struct vnode *root;
    struct filesystem *fs;
};

struct filesystem {
    const char *name;
    int (*setup_mount)(struct filesystem *fs, struct mount *mount);
};

struct file_operations {
    int (*open)(struct vnode *file_node, struct file **target);
    int (*close)(struct file *file);
    int (*read)(struct file *file, void *buf, size_t len);
    int (*write)(struct file *file, const void *buf, size_t len);
    long (*lseek64)(struct file *file, long offset, int whence);
};

struct vnode_operations {
    int (*lookup)(struct vnode *dir_node, struct vnode **target,
                  const char *component_name);
    int (*create)(struct vnode *dir_node, struct vnode **target,
                  const char *component_name);
    int (*mkdir)(struct vnode *dir_node, struct vnode **target,
                 const char *component_name);
};

void vfs_init();
int register_filesystem(struct filesystem *fs);
// int register_chardev(struct file_operations *f_ops);
// int vfs_mknod(char *pathname, int id);
int vfs_open(const char *pathname, int flags, struct file **target);
int vfs_close(struct file *file);
int vfs_read(struct file *file, void *buf, size_t len);
int vfs_write(struct file *file, const void *buf, size_t len);
int vfs_mkdir(const char *pathname);
int vfs_mount(const char *target, const char *filesystem);
int vfs_lookup(const char *pathname, struct vnode **target);

// char *realpath(const char *path, char *resolved_path);

#endif // VFS_H
