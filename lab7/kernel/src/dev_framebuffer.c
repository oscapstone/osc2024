#include "vfs.h"
#include "dev_framebuffer.h"
#include "mini_uart.h"
#include "memory.h"
#include "mbox.h"
#include "utils.h"
#include "exception.h"
#include "stdint.h"
#include "syscall.h"

#define MBOX_CH_PROP 8

// The following code is for mailbox initialize used in lab7.
unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
unsigned char *lfb;                       /* raw frame buffer address */

const struct file_operations dev_framebuffer_operations = {dev_framebuffer_write, (void *)dev_framebuffer_op_deny, dev_framebuffer_open, dev_framebuffer_close, dev_framebuffer_lseek64, (void *)dev_framebuffer_op_deny};
int framebuffer_dev_id = -1;

// https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
int init_dev_framebuffer()
{
    // The following code is for mailbox initialize used in lab7.
    pt[0] = 35 * 4;
    pt[1] = MBOX_TAG_REQUEST_CODE;

    pt[2] = 0x48003; // set phy wh
    pt[3] = 8;
    pt[4] = 8;
    pt[5] = 1024; // FrameBufferInfo.width
    pt[6] = 768;  // FrameBufferInfo.height

    pt[7] = 0x48004; // set virt wh
    pt[8] = 8;
    pt[9] = 8;
    pt[10] = 1024; // FrameBufferInfo.virtual_width
    pt[11] = 768;  // FrameBufferInfo.virtual_height

    pt[12] = 0x48009; // set virt offset
    pt[13] = 8;
    pt[14] = 8;
    pt[15] = 0; // FrameBufferInfo.x_offset
    pt[16] = 0; // FrameBufferInfo.y.offset

    pt[17] = 0x48005; // set depth
    pt[18] = 4;
    pt[19] = 4;
    pt[20] = 32; // FrameBufferInfo.depth

    pt[21] = 0x48006; // set pixel order
    pt[22] = 4;
    pt[23] = 4;
    pt[24] = 1; // RGB, not BGR preferably

    pt[25] = 0x40001; // get framebuffer, gets alignment on request
    pt[26] = 8;
    pt[27] = 8;
    pt[28] = 4096; // FrameBufferInfo.pointer
    pt[29] = 0;    // FrameBufferInfo.size

    pt[30] = 0x40008; // get pitch
    pt[31] = 4;
    pt[32] = 4;
    pt[33] = 0; // FrameBufferInfo.pitch

    pt[34] = MBOX_TAG_LAST_BYTE;

    // this might not return exactly what we asked for, could be
    // the closest supported resolution instead
    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((uint64_t)&pt)) && pt[20] == 32 && pt[28] != 0)
    {
        pt[28] &= 0x3FFFFFFF;                                  // convert GPU address to ARM address
        width = pt[5];                                         // get actual physical width
        height = pt[6];                                        // get actual physical height
        pitch = pt[33];                                        // get number of bytes per line
        isrgb = pt[24];                                        // get the actual channel order
        lfb = ((void *)((uint64_t)pt[28]));                    // raw frame buffer address
    }
    else
    {
        uart_puts("Unable to set screen resolution to 1024x768x32\n");
    }
    dev_t *fb_dev = (dev_t *)kmalloc(sizeof(dev_t));
    fb_dev->name = "framebuffer";
    fb_dev->f_ops = &dev_framebuffer_operations;
    framebuffer_dev_id = register_dev(fb_dev);

    return 0;
}

int dev_framebuffer_write(struct file *file, const void *buf, size_t len)
{
    lock();
    // uart_puts("dev_framebuffer_write: len: %d, f_pos: %d\r\n", len, file->f_pos);
    if (len + file->f_pos > pitch * height)
    {
        uart_puts("How come? dev_framebuffer_write to no where!\r\n");
        len = pitch * height - file->f_pos;
    }
    memcpy(lfb + file->f_pos, buf, len);
    file->f_pos += len;
    unlock();
    return len;
}

int dev_framebuffer_open(struct vnode *file_node, struct file **target)
{
    uart_puts("dev_framebuffer_open\r\n");
    (*target)->f_pos = 0;
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_framebuffer_operations;
    return 0;
}

int dev_framebuffer_close(struct file *file)
{
    kfree(file);
    return 0;
}

int64_t dev_framebuffer_lseek64(struct file *file, int64_t offset, int whence)
{
    uart_puts("dev_framebuffer_lseek64: offset: %d, whence: %d\r\n", offset, whence);
    if (whence == SEEK_SET)
    {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}

int dev_framebuffer_op_deny()
{
    return -1;
}
