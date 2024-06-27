#pragma once

#include "hardware.h"
#include "vfs.h"

extern volatile unsigned int mbox[36];

typedef struct framebuffer_info {
  unsigned int width;
  unsigned int height;
  unsigned int pitch;
  unsigned int isrgb;
} framebuffer_info;

int mbox_call(unsigned char ch, unsigned int *mbox);
int get_board_revision(unsigned int *mbox);
int get_arm_memory_status(unsigned int *mbox);

file_operations *init_dev_framebuffer();

int dev_framebuffer_write(file *f, const void *buf, size_t len);
int dev_framebuffer_open(vnode *file_node, file *target);
int dev_framebuffer_close(file *f);
long dev_framebuffer_lseek64(file *f, long offset, int whence);
