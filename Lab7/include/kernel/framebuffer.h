#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "kernel/vfs.h"
#include "kernel/mailbox.h"
#include "kernel/lock.h"

struct framebuffer_info{
  unsigned int width;
  unsigned int height;
  unsigned int pitch;       // Number of bytes per line (stride)
  unsigned int isrgb;
};

int init_dev_framebuffer();

int framebuffer_write(struct file *file, const void *buf, my_uint64_t len);
int framebuffer_read(struct file *file, void *buf, my_uint64_t len);         // Not implemented, as we don't need to read from framebuffer
int framebuffer_open(struct vnode *file_node, struct file **target);
int framebuffer_close(struct file *file);

#endif