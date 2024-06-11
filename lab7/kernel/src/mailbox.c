#include "gpio.h"
#include "uart.h"
#include "allocator.h"
#include "utils.h"
#include "mailbox.h"

#define MAILBOX_BASE (MMIO_BASE + 0x0000B880)
#define MAILBOX_READ ((volatile unsigned int *)(MAILBOX_BASE + 0x0))
#define MAILBOX_POLL ((volatile unsigned int *)(MAILBOX_BASE + 0x10))
#define MAILBOX_SENDER ((volatile unsigned int *)(MAILBOX_BASE + 0x14))
#define MAILBOX_STATUS ((volatile unsigned int *)(MAILBOX_BASE + 0x18))
#define MAILBOX_CONFIG ((volatile unsigned int *)(MAILBOX_BASE + 0x1C))
#define MAILBOX_WRITE ((volatile unsigned int *)(MAILBOX_BASE + 0x20))
#define MAILBOX_RESPONSE 0x80000000
#define MAILBOX_FULL 0x80000000
#define MAILBOX_EMPTY 0x40000000

#define MBOX_REQUEST 0
#define MBOX_CH_PROP 8
#define MBOX_TAG_LAST 0

volatile unsigned int __attribute__((aligned(16))) mailbox[36];
unsigned int width, height, pitch, isrgb, size; /* dimensions and channel order */
unsigned char *lfb;                             /* raw frame buffer address */

static struct file_operations framebufferfs_f_ops = {
    .write = framebufferfs_write,
    .read = framebufferfs_read};

static struct inode_operations framebufferfs_i_ops = {
    .lookup = framebufferfs_lookup,
    .create = framebufferfs_create,
    .mkdir = framebufferfs_mkdir};

int framebufferfs_setup_mount(struct filesystem *fs, struct mount *mount)
{
    mount->root = kmalloc(sizeof(struct dentry));
    my_strcpy(mount->root->d_name, "/");
    mount->root->d_parent = NULL;

    mount->root->d_inode = kmalloc(sizeof(struct inode));
    mount->root->d_inode->f_ops = &framebufferfs_f_ops;
    mount->root->d_inode->i_ops = &framebufferfs_i_ops;
    mount->root->d_inode->i_dentry = mount->root;
    mount->root->d_inode->internal = NULL;

    for (int i = 0; i < 16; i++)
        mount->root->d_subdirs[i] = NULL;

    mount->fs = fs;

    mailbox[0] = 35 * 4;
    mailbox[1] = MBOX_REQUEST;

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

    mailbox[34] = MBOX_TAG_LAST;

    if (mailbox_call(MBOX_CH_PROP) && mailbox[20] == 32 && mailbox[28] != 0)
    {
        mailbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
        width = mailbox[5];        // get actual physical width
        height = mailbox[6];       // get actual physical height
        pitch = mailbox[33];       // get number of bytes per line
        isrgb = mailbox[24];       // get the actual channel order
        size = mailbox[29];        // get frame buffer size
        lfb = (void *)((unsigned long)mailbox[28] | VA_START);
    }
    else
        uart_puts("Unable to set screen resolution to 1024x768x32\n");

    return 1;
}

int framebufferfs_write(struct file *file, const void *buf, size_t len)
{
    unsigned char *target = (unsigned char *)buf;
    for (int i = 0; i < len; i++)
    {
        if(file->f_pos < size)
        {
            *(lfb + file->f_pos) = target[i];
            ++file->f_pos;
        }
        else
            return i;
    }

    return len;
}

int framebufferfs_read(struct file *file, void *buf, size_t len)
{
    return -1;
}

struct dentry *framebufferfs_lookup(struct inode *dir, const char *component_name)
{
    return NULL;
}

int framebufferfs_create(struct inode *dir, struct dentry *dentry, int mode)
{
    return -1;
}

int framebufferfs_mkdir(struct inode *dir, struct dentry *dentry)
{
    return -1;
}

int mailbox_call(unsigned char ch)
{
    unsigned int r = ((unsigned int)((unsigned long)&mailbox) | (ch & 0xF)); // Combine the message

    while (*MAILBOX_STATUS & MAILBOX_FULL)
        ; // Wait until Mailbox 0 status register's full flag is unset.

    *MAILBOX_WRITE = r; // Write to Mailbox 0 Read/Write register.

    while (1)
    {
        while (*MAILBOX_STATUS & MAILBOX_EMPTY)
            ;                   // Wait until Mailbox 0 status register's empty flag is unset.
        if (r == *MAILBOX_READ) // Read from Mailbox 0 Read/Write register and check if the value is same as before.
            return mailbox[1] == MAILBOX_RESPONSE;
    }

    return 0;
}