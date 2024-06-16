#ifndef _TMPFS_H_
#define _TMPFS_H_

#include "stddef.h"
#include "vfs.h"

struct tmpfs_inode
{
    enum fsnode_type type;
    char name[MAX_FILE_NAME];
    struct vnode *entry[MAX_DIR_ENTRY];
    char *data;
    size_t datasize;
};

int register_tmpfs();
int tmpfs_setup_mount(struct filesystem *fs, struct mount *_mount);

int tmpfs_write(struct file *file, const void *buf, size_t len);
int tmpfs_read(struct file *file, void *buf, size_t len);
int tmpfs_open(struct vnode *file_node, struct file **target);
int tmpfs_close(struct file *file);
long tmpfs_lseek64(struct file *file, long offset, int whence);

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);
long tmpfs_getsize(struct vnode *vd);

// struct vnode *tmpfs_create_vnode(struct mount *_mount, enum fsnode_type type);
struct vnode *tmpfs_create_vnode(enum fsnode_type type);

#endif /* _TMPFS_H_ */
