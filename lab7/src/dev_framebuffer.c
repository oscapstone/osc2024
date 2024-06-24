#include "dev_framebuffer.h"
#include "irq.h"
#include "mbox.h"
#include "string.h"
#include "uart.h"
#include "vm.h"

struct file_operations dev_fb_fops = {
    .open = dev_fb_open,
    .close = dev_fb_close,
    .read = dev_fb_read,
    .write = dev_fb_write,
    .lseek64 = dev_fb_lseek64,
};

unsigned int width, height, pitch, isrgb;
unsigned char *lfb;

int dev_fb_register()
{
    unsigned int __attribute__((aligned(16))) mbox[36];

    mbox[0] = 35 * 4;
    mbox[1] = REQUEST_CODE;

    mbox[2] = 0x48003;
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = 1024;
    mbox[6] = 768;

    mbox[7] = 0x48004;
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = 1024;
    mbox[11] = 768;

    mbox[12] = 0x48009;
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0;
    mbox[16] = 0;

    mbox[17] = 0x48005;
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32;

    mbox[21] = 0x48006;
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1;

    mbox[25] = 0x40001;
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096;
    mbox[29] = 0;

    mbox[30] = 0x40008;
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0;

    mbox[34] = END_TAG;

    if (mbox_call(MAILBOX_CH_PROP, mbox) && mbox[20] == 32 && mbox[28] != 0) {
        mbox[28] &= 0x3FFFFFFF; // Convert GPU address to ARM address
        width = mbox[5];        // Get actual physical width
        height = mbox[6];       // Get actual physical height
        pitch = mbox[33];       // Get number of bytes per line
        isrgb = mbox[24];       // Get the actual channel order
        lfb = PHYS_TO_VIRT((void *)((unsigned long)mbox[28]));
    } else {
        uart_puts("Unable to set screen resolution to 1024x768x32\n");
    }

    return register_device(&dev_fb_fops);
}

int dev_fb_open(struct vnode *file_node, struct file **target)
{
    (*target)->f_pos = 0;
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_fb_fops;
    return 0;
}

int dev_fb_close(struct file *file)
{
    // kfree(file);
    return 0;
}

int dev_fb_read(struct file *file, void *buf, size_t len)
{
    return -1;
}

int dev_fb_write(struct file *file, const void *buf, size_t len)
{
    disable_interrupt();
    if (file->f_pos + len > pitch * height) {
        uart_puts("[ERROR] Framebuffer buffer overflow!\n");
        len = pitch * height - file->f_pos;
    }
    memcpy(lfb + file->f_pos, buf, len);
    file->f_pos += len;
    enable_interrupt();
    return len;
}

long dev_fb_lseek64(struct file *file, long offset, int whence)
{
    if (whence == SEEK_SET) {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}
