#include "fs/framebufferfs.hpp"

#include "board/mailbox.hpp"
#include "io.hpp"
#include "mm/mmu.hpp"
#include "mm/vmm.hpp"

namespace framebufferfs {

int File::ioctl(unsigned long request, void* arg) {
  if (request != 0)
    return -1;

  auto info = (framebuffer_info*)arg;
  info->width = get()->data->width;
  info->height = get()->data->height;
  info->pitch = get()->data->pitch;
  info->isrgb = get()->data->isrgb;

  return 0;
}

FileSystem::FileSystem() {
  unsigned int __attribute__((aligned(16))) mbox[36];
  mbox[0] = 35 * 4;
  mbox[1] = MBOX_REQUEST_CODE;

  mbox[2] = 0x48003;  // set phy wh
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 1024;  // FrameBufferInfo.width
  mbox[6] = 768;   // FrameBufferInfo.height

  mbox[7] = 0x48004;  // set virt wh
  mbox[8] = 8;
  mbox[9] = 8;
  mbox[10] = 1024;  // FrameBufferInfo.virtual_width
  mbox[11] = 768;   // FrameBufferInfo.virtual_height

  mbox[12] = 0x48009;  // set virt offset
  mbox[13] = 8;
  mbox[14] = 8;
  mbox[15] = 0;  // FrameBufferInfo.x_offset
  mbox[16] = 0;  // FrameBufferInfo.y.offset

  mbox[17] = 0x48005;  // set depth
  mbox[18] = 4;
  mbox[19] = 4;
  mbox[20] = 32;  // FrameBufferInfo.depth

  mbox[21] = 0x48006;  // set pixel order
  mbox[22] = 4;
  mbox[23] = 4;
  mbox[24] = 1;  // RGB, not BGR preferably

  mbox[25] = MBOX_ALLOCATE_BUFFER;
  mbox[26] = 8;
  mbox[27] = 8;
  mbox[28] = 4096;  // FrameBufferInfo.pointer
  mbox[29] = 0;     // FrameBufferInfo.size

  mbox[30] = 0x40008;  // get pitch
  mbox[31] = 4;
  mbox[32] = 4;
  mbox[33] = 0;  // FrameBufferInfo.pitch

  mbox[34] = MBOX_END_TAG;

  // this might not return exactly what we asked for, could be
  // the closest supported resolution instead
  if (mailbox_call(MBOX_CH_PROP, (MboxBuf*)mbox) && mbox[20] == 32 &&
      mbox[28] != 0) {
    mbox[28] &= 0x3FFFFFFF;  // convert GPU address to ARM address
    data = new framebuffer_data{
        .width = mbox[5],   // get actual physical width
        .height = mbox[6],  // get actual physical height
        .pitch = mbox[33],  // get number of bytes per line
        .isrgb = mbox[24],  // get the actual channel order
        .lfb = pa2va((char*)(void*)(unsigned long)mbox[28]),
        .buf_size = mbox[29],
    };
    klog("[framebufferfs] width = %d\n", data->width);
    klog("[framebufferfs] height = %d\n", data->height);
    klog("[framebufferfs] pitch = %d\n", data->pitch);
    klog("[framebufferfs] isrgb = %d\n", data->isrgb);
    klog("[framebufferfs] lfb = %p\n", data->lfb);
    klog("[framebufferfs] buf_size = 0x%lx\n", data->buf_size);
  } else {
    data = nullptr;
    kputs("Unable to set screen resolution to 1024x768x32\n");
  }
}

}  // namespace framebufferfs
