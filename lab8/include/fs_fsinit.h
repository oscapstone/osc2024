#ifndef _FSINIT_H
#define _FSINIT_H

#include "fs_vfs.h"
#include "fs_tmpfs.h"
#include "fs_cpio.h"
#include "fs_uartfs.h"
#include "fs_framebufferfs.h"

void fs_early_init(void);
void fs_init(void);

#endif // _FSINIT_H