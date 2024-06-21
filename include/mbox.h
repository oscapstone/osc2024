#ifndef _MBOX_H
#define _MBOX_H

#include "types.h"
#include "vfs.h"

extern volatile uint32_t mbox[36]; /* a properly aligned buffer */

typedef struct framebuffer_info {
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t isrgb;
} framebuffer_info;

void get_arm_base_memory_sz();
void get_board_serial();
void get_board_revision();
uint32_t mbox_call(uint8_t ch);
filesystem *fbfs_init();
#endif