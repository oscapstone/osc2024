#ifndef TMPFS_H
#define TMPFS_H

#include "kernel/vfs.h"
#include "kernel/type.h"
#include "kernel/utils.h"

#define MAX_DIR_ENTRY 16

struct tmpfs_inode {
    char name[MAX_FILE_NAME_LEN];
    //struct tmpfs_node* parent;
    struct vnode* entry[MAX_DIR_ENTRY];
    char *data;
    enum node_type type;
    my_uint64_t data_size;
};

int tmpfs_register();
int tmpfs_setup_mount(struct filesystem *fs, struct mount *mount);
// create a vnode for tmpfs
struct vnode* tmpfs_create_vnode(struct mount* mount, enum node_type type);
long tmpfs_getsize(struct vnode *vd);

int tmpfs_write(struct file *file, const void *buf, my_uint64_t len);
int tmpfs_read(struct file *file, void *buf, my_uint64_t len);
int tmpfs_open(struct vnode *file_node, struct file **target);
int tmpfs_close(struct file *file);

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

#endif