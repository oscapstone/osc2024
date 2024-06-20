#ifndef __TMPFS_H__
#define __TMPFS_H__

#include "vfs.h"

#ifndef DEFAULT_INODE_SIZE
#define DEFAULT_INODE_SIZE             (4096)
#endif


/* tmpfs file system internal node */
struct tmpfs_inode {
    enum fsnode_type type;
    char name[MAX_FILE_NAME_LEN];
    struct vnode *childs[MAX_DIR_NUM];
    char *data;
    unsigned long data_size;
};

extern struct filesystem tmpfs_filesystem;

struct vnode *tmpfs_create_vnode(struct mount *mount, enum fsnode_type type);

#endif // __TMPFS_H__