#ifndef __INITRAMFS_H__
#define __INITRAMFS_H__

#include "vfs.h"

#define DEFAULT_INODE_SIZE             (4096)

/* initramfs file system internal node */
struct initramfs_inode {
    enum fsnode_type type;
    char name[MAX_PATH_LEN];
    struct vnode *childs[MAX_DIR_NUM];
    char *data;
    unsigned long data_size;
};

extern struct filesystem initramfs_filesystem;

int initramfs_setup_mount(struct filesystem* fs, struct mount* mount);

int initramfs_write(struct file *file, const void *buf, size_t len);
int initramfs_read(struct file *file, void *buf, size_t len);
int initramfs_open(struct vnode *file_node, struct file **target);
int initramfs_close(struct file *file);
long initramfs_lseek64(struct file *file, long offset, int whence);

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

struct vnode *initramfs_create_vnode(struct mount *mount, enum fsnode_type type);

#endif // __INITRAMFS_H__