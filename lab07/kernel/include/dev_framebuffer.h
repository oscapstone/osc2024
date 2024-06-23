#ifndef _DEV_FRAMEBUFFER_H__
#define _DEV_FRAMEBUFFER_H__

#include "vfs.h"

struct framebuffer_info
{
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int isrgb;
};

#define SEEK_SET 0

int dev_framebuffer_register();
int  dev_framebuffer_write(struct file *file, const void *buf, size_t len);
int  dev_framebuffer_read(struct file *file, void *buf, size_t len);
int  dev_framebuffer_open(struct vnode *file_node, struct file **target);
int  dev_framebuffer_close(struct file *file);
long dev_framebuffer_lseek64(struct file *file, long offset, int whence);

#endif
