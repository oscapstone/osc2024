#include "mbox.h"

#include "irq.h"
#include "mem.h"
#include "uart.h"
#include "utils.h"
#include "vfs.h"

int mbox_call(unsigned char ch, unsigned int *mbox) {
  unsigned int r = (unsigned int)((unsigned long)mbox & ~0xF) | (ch & 0xF);
  // Wait until we can write to the mailbox
  while (*MAILBOX_REG_STATUS & MAILBOX_FULL)
    ;
  *MAILBOX_REG_WRITE = r; // Write the request
  while (1) {
    // Wait for the response
    while (*MAILBOX_REG_STATUS & MAILBOX_EMPTY)
      ;
    if (r == *MAILBOX_REG_READ)
      return mbox[1] == MAILBOX_RESPONSE;
  }
  return 0;
}

unsigned int __attribute__((aligned(16))) mbox_fb[36];
framebuffer_info fb_info;
uintptr_t lfb; // raw frame buffer addess

int get_board_revision(unsigned int *mbox) {
  mbox[0] = 7 * 4;
  mbox[1] = MAILBOX_REQUEST;
  mbox[2] = TAGS_HARDWARE_BOARD_REVISION;
  mbox[3] = 4;
  mbox[4] = TAG_REQUEST_CODE;
  mbox[5] = 0;
  mbox[6] = END_TAG;
  return mbox_call(MAILBOX_CH_PROP, mbox);
}

int get_arm_memory_status(unsigned int *mbox) {
  mbox[0] = 8 * 4;
  mbox[1] = MAILBOX_REQUEST;
  mbox[2] = TAGS_HARDWARE_ARM_MEM;
  mbox[3] = 8;
  mbox[4] = TAG_REQUEST_CODE;
  mbox[5] = 0;
  mbox[6] = 0;
  mbox[7] = END_TAG;
  return mbox_call(MAILBOX_CH_PROP, mbox);
}

file_operations dev_framebuffer_operation = {
    dev_framebuffer_write,   //
    (void *)not_supported,   // read
    dev_framebuffer_open,    //
    dev_framebuffer_close,   //
    dev_framebuffer_lseek64, //
    (void *)not_supported    // getsize
};

file_operations *init_dev_framebuffer() {
  // The following code is for mailbox initialize used in lab7.
  mbox_fb[0] = 35 * 4;
  mbox_fb[1] = MAILBOX_REQUEST;

  mbox_fb[2] = FB_PHY_WID_HEIGHT_SET; // set phy wh
  mbox_fb[3] = 8;
  mbox_fb[4] = 8;
  mbox_fb[5] = 1024; // FrameBufferInfo.width
  mbox_fb[6] = 768;  // FrameBufferInfo.height

  mbox_fb[7] = FB_VIR_WID_HEIGHT_SET; // set virt wh
  mbox_fb[8] = 8;
  mbox_fb[9] = 8;
  mbox_fb[10] = 1024; // FrameBufferInfo.virtual_width
  mbox_fb[11] = 768;  // FrameBufferInfo.virtual_height

  mbox_fb[12] = FB_VIR_OFFSET_SET; // set virt offset
  mbox_fb[13] = 8;
  mbox_fb[14] = 8;
  mbox_fb[15] = 0; // FrameBufferInfo.x_offset
  mbox_fb[16] = 0; // FrameBufferInfo.y.offset

  mbox_fb[17] = FB_DEPTH_SET; // set depth
  mbox_fb[18] = 4;
  mbox_fb[19] = 4;
  mbox_fb[20] = 32; // FrameBufferInfo.depth

  mbox_fb[21] = FB_PIXEL_ORDER_SET; // set pixel order
  mbox_fb[22] = 4;
  mbox_fb[23] = 4;
  mbox_fb[24] = 1; // RGB, not BGR preferably

  mbox_fb[25] = FB_ALLOC_BUFFER; // get framebuffer, gets alignment on request
  mbox_fb[26] = 8;
  mbox_fb[27] = 8;
  mbox_fb[28] = 4096; // FrameBufferInfo.pointer
  mbox_fb[29] = 0;    // FrameBufferInfo.size

  mbox_fb[30] = FB_PITCH_GET; // get pitch
  mbox_fb[31] = 4;
  mbox_fb[32] = 4;
  mbox_fb[33] = 0; // FrameBufferInfo.pitch

  mbox_fb[34] = END_TAG;

  // this might not return exactly what we asked for, could be
  // the closest supported resolution instead
  if (mbox_call(MAILBOX_CH_PROP, mbox_fb) && mbox_fb[20] == 32 &&
      mbox_fb[28] != 0) {
    mbox_fb[28] &= 0x3FFFFFFF;   // convert GPU address to ARM address
    fb_info.width = mbox_fb[5];  // get actual physical width
    fb_info.height = mbox_fb[6]; // get actual physical height
    fb_info.pitch = mbox_fb[33]; // get number of bytes per line
    fb_info.isrgb = mbox_fb[24]; // get the actual channel order
    lfb = mbox_fb[28];           // get pointer to the framebuffer
  } else {
    uart_log(ERR, "Unable to set screen resolution to 1024x768x32\n");
  }

  return &dev_framebuffer_operation;
}

int dev_framebuffer_write(file *f, const void *buf, size_t len) {
  disable_interrupt();
  if (len + f->f_pos > fb_info.pitch * fb_info.height) {
    uart_log(WARN, "Buffer overflow\n");
    len = fb_info.pitch * fb_info.height - f->f_pos;
  }
  memcpy((void *)(TO_VIRT(lfb) + f->f_pos), buf, len);
  f->f_pos += len;
  enable_interrupt();
  return len;
}

int dev_framebuffer_open(vnode *file_node, file *target) {
  target->f_pos = 0;
  target->vnode = file_node;
  target->f_ops = file_node->f_ops;
  return 0;
}

int dev_framebuffer_close(file *f) {
  kfree(f, SILENT);
  return 0;
}

long dev_framebuffer_lseek64(file *f, long offset, int whence) {
  if (whence == SEEK_SET) {
    f->f_pos = offset;
    return f->f_pos;
  } else if (whence == SEEK_CUR) {
    long size = f->f_ops->getsize(f->vnode);
    f->f_pos += offset;
    if (f->f_pos > size)
      f->f_pos = size;
  } else
    return not_supported();
  return f->f_pos;
}
