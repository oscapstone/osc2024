#include "vfs.h"
#include "dev_framebuffer.h"
#include "io.h"
#include "alloc.h"
#include "mailbox.h"
#include "peripherals/mailbox.h"
#include "string.h"


extern void disable_irq();
extern void enable_irq();

//The following code is for mailbox initialize used in lab7.
unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
unsigned char *lfb;                       /* raw frame buffer address */

struct file_operations dev_framebuffer_operations = {
    dev_framebuffer_write,
    0, // read
    dev_framebuffer_open, 
    dev_framebuffer_close,
    0, // getsize
    dev_framebuffer_lseek64
};

unsigned int __attribute__((aligned(16))) mbox_s[36];

int dev_framebuffer_register()
{
    printf("Registering framebuffer device...");
    //The following code is for mailbox initialize used in lab7.
    mbox_s[0] = 35 * 4;
    mbox_s[1] = TAG_REQUEST_CODE;

    mbox_s[2] = 0x48003; // set phy wh
    mbox_s[3] = 8;
    mbox_s[4] = 8;
    mbox_s[5] = 1024; // FrameBufferInfo.width
    mbox_s[6] = 768;  // FrameBufferInfo.height

    mbox_s[7] = 0x48004; // set virt wh
    mbox_s[8] = 8;
    mbox_s[9] = 8;
    mbox_s[10] = 1024; // FrameBufferInfo.virtual_width
    mbox_s[11] = 768;  // FrameBufferInfo.virtual_height

    mbox_s[12] = 0x48009; // set virt offset
    mbox_s[13] = 8;
    mbox_s[14] = 8;
    mbox_s[15] = 0; // FrameBufferInfo.x_offset
    mbox_s[16] = 0; // FrameBufferInfo.y.offset

    mbox_s[17] = 0x48005; // set dembox_sh
    mbox_s[18] = 4;
    mbox_s[19] = 4;
    mbox_s[20] = 32; // FrameBufferInfo.dembox_sh

    mbox_s[21] = 0x48006; // set pixel order
    mbox_s[22] = 4;
    mbox_s[23] = 4;
    mbox_s[24] = 1; // RGB, not BGR preferably

    mbox_s[25] = 0x40001; // get framebuffer, gets alignment on request
    mbox_s[26] = 8;
    mbox_s[27] = 8;
    mbox_s[28] = 4096; // FrameBufferInfo.pointer
    mbox_s[29] = 0;    // FrameBufferInfo.size

    mbox_s[30] = 0x40008; // get pitch
    mbox_s[31] = 4;
    mbox_s[32] = 4;
    mbox_s[33] = 0; // FrameBufferInfo.pitch

    mbox_s[34] = MBOX_TAG_LAST;

    // this might not return exactly what we asked for, could be
    // the closest supported resolution instead
    if (mailbox_call_s(MBOX_CH_PROP, mbox_s) && mbox_s[20] == 32 && mbox_s[28] != 0)
    {
        mbox_s[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
        width = mbox_s[5];        // get actual physical width
        height = mbox_s[6];       // get actual physical height
        pitch = mbox_s[33];       // get number of bytes per line
        isrgb = mbox_s[24];       // get the actual channel order
        lfb = ((void *)((unsigned long)mbox_s[28])); // raw frame buffer address
    }
    else
    {
        printf("Unable to set screen resolution to 1024x768x32\n");
    }

    return register_dev(&dev_framebuffer_operations);
}

int dev_framebuffer_write(struct file *file, const void *buf, size_t len)
{
    enable_irq();
    if (len + file->f_pos > pitch * height)
    {
        printf("\r\n[ERROR] Invlaid Write framebuffer!");
        len = pitch * height - file->f_pos;
    }
    for(int i=0; i<len; i++){
        ((char*)(lfb + file->f_pos))[i] = ((char*)buf)[i];
    }
    file->f_pos += len;
    disable_irq();
    return len;
}

int dev_framebuffer_open(struct vnode *file_node, struct file **target)
{
    printf("\r\n[INFO] Opening framebuffer device...");
    (*target)->f_pos = 0;
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_framebuffer_operations;
    return 0;
}

int dev_framebuffer_close(struct file *file)
{
    dfree(file);
    return 0;
}

long dev_framebuffer_lseek64(struct file *file, long offset, int whence)
{
    if (whence == SEEK_SET)
    {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}
