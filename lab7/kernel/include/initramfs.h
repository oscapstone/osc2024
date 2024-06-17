#ifndef INITRAMFS_H
#define INITRAMFS_H

#include "vfs.h"

typedef struct initramfs_node{
    char name[MAX_PATH_SIZE];
    struct vnode * entry[MAX_ENTRY_SIZE];
    char * data; // simply point to the place in initramfs
    int size; // get by the code
} initramfs_node;

int reg_initramfs();
int initramfs_write(struct file *file, const void *buf, size_t len);
int initramfs_read(struct file *file, void *buf, size_t len);
int initramfs_open(struct vnode *file_node, struct file **target);
int initramfs_close(struct file *file);

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

int initramfs_mount (struct filesystem *_fs, struct mount *mt);

#endif
