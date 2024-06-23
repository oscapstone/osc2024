#ifndef _FS_UARTFS_H
#define _FS_UARTFS_H

#include "fs_vfs.h"
#include "mini_uart.h"

filesystem *uartfs_init(void);

int uartfs_mount(filesystem *fs, mount *mnt);

extern filesystem static_uartfs;


int uartfs_lookup(vnode *dir_node, vnode **target,
                        const char *component_name);
int uartfs_create(vnode *dir_node, vnode **target,
                        const char *component_name);
int uartfs_mkdir(vnode *dir_node, vnode **target,
                       const char *component_name);
int uartfs_isdir(vnode *dir_node);
int uartfs_getname(vnode *dir_node, const char **name);
int uartfs_getsize(vnode *dir_node);

extern vnode_operations uartfs_v_ops;

int uartfs_open(vnode *file_node, file *target);
int uartfs_close(file *target);
int uartfs_write(file *target, const void *buf, size_t len);
int uartfs_read(file *target, void *buf, size_t len);
long uartfs_lseek64(file *target, long offset, int whence);

extern file_operations uartfs_f_ops;

typedef struct uartfs_internal {
    const char *name;
    vnode oldnode;
} uartfs_internal;


#endif // _FS_UARTFS_H