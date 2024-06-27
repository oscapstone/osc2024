#ifndef __DEV_FRAMEBUFFER_H__
#define __DEV_FRAMEBUFFER_H__

#include "vfs.h"

struct framebuffer_info
{
  unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
};

int framebuffer_init();

int dev_framebuffer_write(struct file *file, const void *buf, unsigned int len);
int dev_framebuffer_read(struct file *file, void *buf, unsigned int len);
int dev_framebuffer_open(struct vnode *node, struct file **file);
int dev_framebuffer_close(struct file *file);
long dev_framebuffer_lseek64(struct file *file, long offset, int whence);

int dev_framebuffer_ioctl(struct framebuffer_info *info);

#endif