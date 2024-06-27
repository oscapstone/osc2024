#ifndef VFS_TMPFS_H
#define VFS_TMPFS_H

#include "vfs.h"

#define TMPFS_MAX_FILE_NAME 15
#define TMPFS_MAX_DIR_ENTRY 16
#define TMPFS_MAX_FILE_SIZE 4096

struct tmpfs_vnode {
    enum fsnode_type type;
    char name[TMPFS_MAX_FILE_NAME];
    struct vnode *entry[TMPFS_MAX_DIR_ENTRY];
    char *data;
    size_t datasize;
};

int tmpfs_register();
int tmpfs_setup_mount(struct filesystem *fs, struct mount *mnt);
int tmpfs_open(struct vnode *file_node, struct file **target);
int tmpfs_close(struct file *file);
int tmpfs_read(struct file *file, void *buf, size_t len);
int tmpfs_write(struct file *file, const void *buf, size_t len);
long tmpfs_lseek64(struct file *file, long offset, int whence);
int tmpfs_lookup(struct vnode *dir_node, struct vnode **target,
                 const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target,
                 const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target,
                const char *component_name);

#endif // VFS_TMPFS_H
