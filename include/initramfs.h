#ifndef __INITRAMFS_H__
#define __INITRAMFS_H__

#include "vfs.h"

/* initramfs file system internal node */
struct initramfs_inode {
    enum fsnode_type type;
    char name[MAX_FILE_NAME_LEN];
    struct vnode *childs[MAX_DIR_NUM];
    char *data;
    unsigned long data_size;
};

extern struct filesystem initramfs_filesystem;

struct vnode *initramfs_create_vnode(struct mount *mount, enum fsnode_type type);

#endif // __INITRAMFS_H__