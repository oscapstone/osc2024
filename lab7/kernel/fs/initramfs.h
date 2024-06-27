#ifndef _INITRAMFS_H_
#define _INITRAMFS_H_

#include <lib/stddef.h>
#include <kernel/fs/vfs.h>

#define INITRAMFS_MAX_DIR_ENTRY 100

// initramfs basically is same as tmpfs, but it is read-only

struct initramfs_inode
{
    enum fsnode_type type;
    char name[64];
    struct vnode *entry[INITRAMFS_MAX_DIR_ENTRY];
    char *data;
    size_t datasize;
};

int register_initramfs();
int initramfs_setup_mount(struct filesystem *fs, struct mount *_mount);

int initramfs_write(struct file *file, const void *buf, size_t len);
int initramfs_read(struct file *file, void *buf, size_t len);
int initramfs_open(struct vnode *file_node, struct file **target);
int initramfs_close(struct file *file);
long initramfs_lseek64(struct file *file, long offset, int whence);
long initramfs_getsize(struct vnode *vd);

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

struct vnode *initramfs_create_vnode(struct mount *_mount, enum fsnode_type type);

#endif /* _INITRAMFS_H_ */
