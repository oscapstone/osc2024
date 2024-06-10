#ifndef __INITRAMFS_H__
#define __INITRAMFS_H__

#define COMPONENT_SIZE  15
#define ENTRIES_PER_DIR 16
#define INITRAMFS_MAX_FILE  4096

#include "vfs.h"
#include "cpio.h"

struct initramfs_internal {
    char* name; // file name or directory name
    unsigned long size; // child count for directory
    char* data;
    unsigned long filesize;
    int type;   // DIR_NODE or FILE_NODE
    // struct vnode* parent;
    struct vnode* children[ENTRIES_PER_DIR];
    // struct vnode* next;
};

int initramfs_list(struct vnode* dir_node);
int initramfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name);
int initramfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name);
int initramfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name);
int initramfs_close(struct file* file);
int initramfs_open(struct vnode* file_node, struct file** target);
int initramfs_read(struct file* file, void* buf, size_t len);
int initramfs_write(struct file* file, const void* buf, size_t len);
int initramfs_setup_mount(struct filesystem* fs, struct mount** mount);
int initramfs_register();
// int parse_initramfs();

#endif