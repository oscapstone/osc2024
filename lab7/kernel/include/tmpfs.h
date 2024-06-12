#ifndef _TMPFS_H_
#define _TMPFS_H_

#include "stddef.h"
#include "vfs.h"
#include "list.h"

// SPEC basic Note #3
#define MAX_FILE_SIZE 4096

typedef struct tmpfs_inode
{
    vnode_list_t *child_list;
    char name[MAX_FILE_NAME];       // Name              : represents file name
    char *data;
    size_t datasize;
}tmpfs_inode_t;

int register_tmpfs();
filesystem_t *get_tmpfs();
int tmpfs_setup_mount(filesystem_t *fs, mount_t *_mount, vnode_t *parent, const char *name);
vnode_t *tmpfs_create_vnode(mount_t *superblock, enum fsnode_type type, vnode_t *parent, const char *name);
tmpfs_inode_t *tmpfs_create_inode(enum fsnode_type type, const char *name);


int tmpfs_write(struct file *file, const void *buf, size_t len);
int tmpfs_read(struct file *file, void *buf, size_t len);
int tmpfs_open(struct vnode *file_node, struct file **target);
int tmpfs_close(struct file *file);
long tmpfs_lseek64(struct file *file, long offset, int whence);
long tmpfs_getsize(struct vnode *vd);

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_readdir(struct vnode *dir_node, const char name_array[]);

vnode_t *tmpfs_create_vnode(mount_t *_mount, enum fsnode_type type, vnode_t *parent, const char *name);

#endif /* _TMPFS_H_ */
