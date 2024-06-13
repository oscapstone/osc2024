#include "kernel/framebuffer.h"

#define mailbox_REQUEST 0
#define mailbox_CH_PROP 8
#define mailbox_TAG_LAST 0

//unsigned int __attribute__((aligned(16))) mailbox[36];
unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
unsigned char *lfb;                       /* raw frame buffer address */

struct file_operations framebuffer_f_ops = {framebuffer_write, op_denied, framebuffer_open, framebuffer_close, vfs_lseek64, op_denied};

int init_dev_framebuffer(){
    mailbox[0] = 35 * 4;
    mailbox[1] = mailbox_REQUEST;

    mailbox[2] = 0x48003; // set phy wh
    mailbox[3] = 8;
    mailbox[4] = 8;
    mailbox[5] = 1024; // FrameBufferInfo.width
    mailbox[6] = 768;  // FrameBufferInfo.height

    mailbox[7] = 0x48004; // set virt wh
    mailbox[8] = 8;
    mailbox[9] = 8;
    mailbox[10] = 1024; // FrameBufferInfo.virtual_width
    mailbox[11] = 768;  // FrameBufferInfo.virtual_height

    mailbox[12] = 0x48009; // set virt offset
    mailbox[13] = 8;
    mailbox[14] = 8;
    mailbox[15] = 0; // FrameBufferInfo.x_offset
    mailbox[16] = 0; // FrameBufferInfo.y.offset

    mailbox[17] = 0x48005; // set depth
    mailbox[18] = 4;
    mailbox[19] = 4;
    mailbox[20] = 32; // FrameBufferInfo.depth

    mailbox[21] = 0x48006; // set pixel order
    mailbox[22] = 4;
    mailbox[23] = 4;
    mailbox[24] = 1; // RGB, not BGR preferably

    mailbox[25] = 0x40001; // get framebuffer, gets alignment on request
    mailbox[26] = 8;
    mailbox[27] = 8;
    mailbox[28] = 4096; // FrameBufferInfo.pointer
    mailbox[29] = 0;    // FrameBufferInfo.size

    mailbox[30] = 0x40008; // get pitch
    mailbox[31] = 4;
    mailbox[32] = 4;
    mailbox[33] = 0; // FrameBufferInfo.pitch

    mailbox[34] = mailbox_TAG_LAST;

    // this might not return exactly what we asked for, could be
    // the closest supported resolution instead
    if (mailbox_call(mailbox_CH_PROP, mailbox) && mailbox[20] == 32 && mailbox[28] != 0){
        mailbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
        width = mailbox[5];        // get actual physical width
        height = mailbox[6];       // get actual physical height
        pitch = mailbox[33];       // get number of bytes per line
        isrgb = mailbox[24];       // get the actual channel order
        lfb = (void *)((unsigned long)mailbox[28]);    // set lfb to start of raw framebuffer address
    } 
    else{
        uart_puts("Unable to set screen resolution to 1024x768x32\n");
    }

    return register_devfs(&framebuffer_f_ops);
}

int framebuffer_write(struct file *file, const void *buf, my_uint64_t len){
    lock();
    if(file->f_pos + len > pitch * height){
        uart_puts("Framebuffer write out of bounds\n");
        len = pitch * height - file->f_pos;
    }
    memcpy(lfb + file->f_pos, buf, len);
    file->f_pos += len;
    unlock();
    return len;
}

int framebuffer_open(struct vnode *file_node, struct file **target){
    (*target)->f_ops = &framebuffer_f_ops;
    (*target)->vnode = file_node;
    (*target)->f_pos = 0;
    return 0;
}
int framebuffer_close(struct file *file){
    pool_free(file);
    return 0;
}