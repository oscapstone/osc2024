#ifndef __TMPFS_H__
#define __TMPFS_H__

#define COMPONENT_SIZE  15
#define ENTRIES_PER_DIR 16
#define TMPFS_MAX_FILE  4096

#include "vfs.h"

struct tmpfs_internal {
    char* name; // file name or directory name
    unsigned long size;
    char* data;
    int type;   // DIR_NODE or FILE_NODE
    // struct vnode* parent;
    struct vnode* children[ENTRIES_PER_DIR];
    // struct vnode* next;
};

int tmpfs_list(struct vnode* dir_node);
int tmpfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name);
int tmpfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name);
int tmpfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name);
int tmpfs_close(struct file* file);
int tmpfs_open(struct vnode* file_node, struct file** target);
int tmpfs_read(struct file* file, void* buf, size_t len);
int tmpfs_write(struct file* file, const void* buf, size_t len);
int tmpfs_setup_mount(struct filesystem* fs, struct mount** mount);
int tmpfs_register();

#endif