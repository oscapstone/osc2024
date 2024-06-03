#pragma once

#include "base.h"

typedef struct _FRAMEBUFFER_INFO
{
    U32 width;
    U32 height;
    U32 pitch;
    U32 isrgb;
}FRAMEBUFFER_INFO;


typedef struct _FRAMEBUFFER_MANAGER
{
    FRAMEBUFFER_INFO info;
    UPTR lfb_addr;
}FRAMEBUFFER_MANAGER;

extern FRAMEBUFFER_MANAGER framebuffer_manager;

