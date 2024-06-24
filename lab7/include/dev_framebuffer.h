#ifndef DEV_FRAMEBUFFER_H
#define DEV_FRAMEBUFFER_H

#include "vfs.h"

struct framebuffer_info {
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int isrgb;
};

int dev_fb_register();
int dev_fb_open(struct vnode *file_node, struct file **target);
int dev_fb_close(struct file *file);
int dev_fb_read(struct file *file, void *buf, size_t len);
int dev_fb_write(struct file *file, const void *buf, size_t len);
long dev_fb_lseek64(struct file *file, long offset, int whence);

#endif // DEV_FRAMEBUFFER_H
