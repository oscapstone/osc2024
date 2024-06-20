#include "vfs.h"
#include "dev_framebuffer.h"
#include "uart1.h"
#include "memory.h"
#include "mbox.h"
#include "string.h"
#include "exception.h"

#define MBOX_CH_PROP 8

//The following code is for mailbox initialize used in lab7.
unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
unsigned char *lfb;                       /* raw frame buffer address */

struct file_operations dev_framebuffer_operations = {dev_framebuffer_write, (void *)dev_framebuffer_op_deny, dev_framebuffer_open, dev_framebuffer_close, dev_framebuffer_lseek64, (void *)dev_framebuffer_op_deny};

// VFS為framebuffer分配一個ID, VFS就可辨識device
// 初始化framebuffer並將其註冊到 VFS 中
// https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
int init_dev_framebuffer()
{
    //The following code is for mailbox initialize used in lab7.
    // 設置 mailbox buffer，set framebuffer的attribute(width, height, depth, RGB, etc.)
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
    // call mailbox，並檢查返回值是否有效。
    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)) && pt[20] == 32 && pt[28] != 0)
    {
        pt[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
        width = pt[5];        // get actual physical width
        height = pt[6];       // get actual physical height
        pitch = pt[33];       // get number of bytes per line
        isrgb = pt[24];       // get the actual channel order
        lfb = PHYS_TO_VIRT((void *)((unsigned long)pt[28])); // raw frame buffer address
    }
    else
    {
        uart_puts("Unable to set screen resolution to 1024x768x32\n");
    }

    return register_dev(&dev_framebuffer_operations);
}

// 將data寫入 framebuffer
int dev_framebuffer_write(struct file *file, const void *buf, size_t len)
{
    lock();
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

// 打開 framebuffer file，初始化file descriptors table
int dev_framebuffer_open(struct vnode *file_node, struct file **target)
{
    (*target)->f_pos = 0;
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_framebuffer_operations;
    return 0;
}

// 關閉 framebuffer file，release資源
int dev_framebuffer_close(struct file *file)
{
    kfree(file);
    return 0;
}

// 設置file pointer position
long dev_framebuffer_lseek64(struct file *file, long offset, int whence)
{
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
