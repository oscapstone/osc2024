#ifndef _FBDEV_H_
#define _FBDEV_H_

#include <lib/stddef.h>

struct framebuffer_info {
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int isrgb;
};

int fb_init();
int fb_write(size_t pos, const void *buf, size_t len);

#endif
