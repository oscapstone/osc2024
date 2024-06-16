#ifndef __TMPFS_H__
#define __TMPFS_H__

#include "vfs.h"

#define DEFAULT_INODE_SIZE             (4096)

/* tmpfs file system internal node */
struct tmpfs_inode {
    enum fsnode_type type;
    char name[MAX_PATH_LEN];
    struct vnode *childs[MAX_DIR_NUM];
    char *data;
    unsigned long data_size;
};

extern struct filesystem tmpfs_filesystem;

int tmpfs_write(struct file *file, const void *buf, size_t len);
int tmpfs_read(struct file *file, void *buf, size_t len);
int tmpfs_open(struct vnode *file_node, struct file **target);
int tmpfs_close(struct file *file);
long tmpfs_lseek64(struct file *file, long offset, int whence);

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

struct vnode *tmpfs_create_vnode(struct mount *mount, enum fsnode_type type);


#endif // __TMPFS_H__