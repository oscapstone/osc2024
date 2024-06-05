
#include "lib/mbox_call.h"
#include "lib/uartwrite.h"
#include "lib/printf.h"
#include "lib/open.h"
#include "lib/close.h"

struct framebuffer_info {
  unsigned int width;
  unsigned int height;
  unsigned int pitch;
  unsigned int isrgb;
};

int main() {

    int fd = open("/dev/framebuffer", 2);

    struct framebuffer_info fb_info;
    fb_info.width = 1024;
    fb_info.height = 768;
    fb_info.pitch = 4096;
    fb_info.isrgb = 1;

    ioctl(fd, 0, &fb_info);

    char color[4];

    if (fb_info.isrgb) {
        color[0] = 255;
        color[1] = 0;
        color[2] = 0;
        color[3] = 255;
    } else {
        color[0] = 0;
        color[1] = 0;
        color[2] = 255;
        color[3] = 255;
    }

    for (int y = 0; y < 5; y++) {
        lseek64(fd, y * fb_info.width * 4, 0/* SEEK_SET */);
        for (int x = 0; x < 5; x++) {
            write(fd, color, 4);
        }
    }

    close(fd);

    return 0;
}

