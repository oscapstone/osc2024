#ifndef _INITRAMFS_H_
#define _INITRAMFS_H_

#include "stddef.h"
#include "vfs.h"
#include "list.h"

#define INITRAMFS_PATH "/initramfs"
#define KERNEL_PATH "/initramfs/kernel8.img"

typedef struct initramfs_inode
{
    vnode_list_t *child_list;
    char name[MAX_FILE_NAME]; // Name              : represents file name
    char *data;
    size_t datasize;
} initramfs_inode_t;

int register_initramfs();
int initramfs_setup_mount(filesystem_t *fs, mount_t *_mount, vnode_t *parent, const char *name);
vnode_t *initramfs_create_vnode(mount_t *superblock, enum fsnode_type type, vnode_t *parent, const char *name, char *data, size_t datasize);
initramfs_inode_t *initramfs_create_inode(enum fsnode_type type, const char *name, char *data, size_t datasize);

int initramfs_write(struct file *file, const void *buf, size_t len);
int initramfs_read(struct file *file, void *buf, size_t len);
int initramfs_open(struct vnode *file_node, struct file **target);
int initramfs_close(struct file *file);
long initramfs_lseek64(struct file *file, long offset, int whence);
long initramfs_getsize(struct vnode *vd);

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);
int __initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_readdir(struct vnode *dir_node, const char name_array[]);
#endif /* _INITRAMFS_H_ */
