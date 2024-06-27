#include "dev_framebuffer.h"
#include "mailbox.h"
#include "memory.h"
#include "vfs.h"
#include "string.h"
#include "syscall.h"
#include "mini_uart.h"
#include "interrupt.h"

unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
unsigned char *lfb;                       /* raw frame buffer address */

struct file_operations dev_framebuffer_ops = {
    .write = dev_framebuffer_write,
    .read = dev_framebuffer_read,
    .open = dev_framebuffer_open,
    .close = dev_framebuffer_close,
    .lseek64 = dev_framebuffer_lseek64,
};

int framebuffer_init()
{
  unsigned int __attribute__((aligned(16))) mbox[36];

  mbox[0] = 35 * 4;
  mbox[1] = REQUEST_CODE;

  mbox[2] = 0x48003; // set phy wh
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 1024; // FrameBufferInfo.width
  mbox[6] = 768;  // FrameBufferInfo.height

  mbox[7] = 0x48004; // set virt wh
  mbox[8] = 8;
  mbox[9] = 8;
  mbox[10] = 1024; // FrameBufferInfo.virtual_width
  mbox[11] = 768;  // FrameBufferInfo.virtual_height

  mbox[12] = 0x48009; // set virt offset
  mbox[13] = 8;
  mbox[14] = 8;
  mbox[15] = 0; // FrameBufferInfo.x_offset
  mbox[16] = 0; // FrameBufferInfo.y.offset

  mbox[17] = 0x48005; // set depth
  mbox[18] = 4;
  mbox[19] = 4;
  mbox[20] = 32; // FrameBufferInfo.depth

  mbox[21] = 0x48006; // set pixel order
  mbox[22] = 4;
  mbox[23] = 4;
  mbox[24] = 1; // RGB, not BGR preferably

  mbox[25] = 0x40001; // get framebuffer, gets alignment on request
  mbox[26] = 8;
  mbox[27] = 8;
  mbox[28] = 4096; // FrameBufferInfo.pointer
  mbox[29] = 0;    // FrameBufferInfo.size

  mbox[30] = 0x40008; // get pitch
  mbox[31] = 4;
  mbox[32] = 4;
  mbox[33] = 0; // FrameBufferInfo.pitch

  mbox[34] = END_TAG;

  // this might not return exactly what we asked for, could be
  // the closest supported resolution instead
  if (mailbox_call(CHANNEL8, mbox) && mbox[20] == 32 && mbox[28] != 0)
  {
    mbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
    width = mbox[5];        // get actual physical width
    height = mbox[6];       // get actual physical height
    pitch = mbox[33];       // get number of bytes per line
    isrgb = mbox[24];       // get the actual channel order
    lfb = (void *)((unsigned long)mbox[28]);
  }
  else
  {
    uart_sends("Unable to set screen resolution to 1024x768x32\n");
    return -1;
  }
  return 0;
}

int dev_framebuffer_write(struct file *file, const void *buf, unsigned int len)
{
  if (file->f_pos + len > width * height * 4)
    len = width * height * 4 - file->f_pos;

  memcpy(lfb + file->f_pos, (void *)buf, len);
  file->f_pos += len;
  return len;
}

int dev_framebuffer_read(struct file *file, void *buf, unsigned int len)
{
  return -1;
}

int dev_framebuffer_open(struct vnode *node, struct file **file)
{
  (*file)->vnode = node;
  (*file)->f_pos = 0;
  (*file)->f_ops = &dev_framebuffer_ops;
  return 0;
}

int dev_framebuffer_close(struct file *file)
{
  kfree(file);
  return 0;
}

long dev_framebuffer_lseek64(struct file *file, long offset, int whence)
{
  long new_pos = 0;
  switch (whence)
  {
  case SEEK_SET:
    new_pos = offset;
    break;
  default:
    return -1;
  }

  if (new_pos < 0)
    return -1;

  file->f_pos = new_pos;
  return new_pos;
}

int dev_framebuffer_ioctl(struct framebuffer_info *info)
{
  info->width = width;
  info->height = height;
  info->pitch = pitch;
  info->isrgb = isrgb;
  return 0;
}
