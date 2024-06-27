#ifndef _FS_FRAMEBUFFERFS_H
#define _FS_FRAMEBUFFERFS_H

#include "fs_vfs.h"
#include "mbox.h"
#include "string.h"

#define MBOX_REQUEST 0
#define MBOX_CH_PROP 8
#define MBOX_TAG_LAST 0


typedef struct fb_info {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t isrgb;
} fb_info;

filesystem *framebufferfs_init(void);

int framebufferfs_mount(filesystem *fs, mount *mnt);

extern filesystem static_framebufferfs;


int framebufferfs_lookup(vnode *dir_node, vnode **target,
                        const char *component_name);
int framebufferfs_create(vnode *dir_node, vnode **target,
                        const char *component_name);
int framebufferfs_mkdir(vnode *dir_node, vnode **target,
                       const char *component_name);
int framebufferfs_isdir(vnode *dir_node);
int framebufferfs_getname(vnode *dir_node, const char **name);
int framebufferfs_getsize(vnode *dir_node);

extern vnode_operations framebufferfs_v_ops;

int framebufferfs_open(vnode *file_node, file *target);
int framebufferfs_close(file *target);
int framebufferfs_write(file *target, const void *buf, size_t len);
int framebufferfs_read(file *target, void *buf, size_t len);
long framebufferfs_lseek64(file *target, long offset, int whence);
int framebufferfs_ioctl(struct file *file, uint64_t request, va_list args);

extern file_operations framebufferfs_f_ops;

typedef struct framebufferfs_internal {
    const char *name;
    struct vnode oldnode;
    /* raw frame buffer address */
    uint8_t *lfb;
    uint32_t lfbsize;
    int isopened;
    int isinit;
} framebufferfs_internal;
#endif // _FS_FRAMEBUFFERFS_H