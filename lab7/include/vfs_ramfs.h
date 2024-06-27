#ifndef VFS_RAMFS_H
#define VFS_RAMFS_H

#include "vfs.h"

#define RAMFS_MAX_DIR_ENTRY 16
#define RAMFS_MAX_FILE_SIZE 4096

struct ramfs_vnode {
    enum fsnode_type type;
    char *name;
    struct vnode *entry[RAMFS_MAX_DIR_ENTRY];
    char *data;
    size_t datasize;
};

int ramfs_register();
int ramfs_setup_mount(struct filesystem *fs, struct mount *mnt);
int ramfs_open(struct vnode *file_node, struct file **target);
int ramfs_close(struct file *file);
int ramfs_read(struct file *file, void *buf, size_t len);
int ramfs_write(struct file *file, const void *buf, size_t len);
long ramfs_lseek64(struct file *file, long offset, int whence);
int ramfs_lookup(struct vnode *dir_node, struct vnode **target,
                 const char *component_name);
int ramfs_create(struct vnode *dir_node, struct vnode **target,
                 const char *component_name);
int ramfs_mkdir(struct vnode *dir_node, struct vnode **target,
                const char *component_name);

#endif // VFS_INITRAMFS_H
