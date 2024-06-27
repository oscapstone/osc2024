#ifndef __BASE_H__
#define __BASE_H__

#include "vm.h"

#define MMU_BASE (VIRT_OFFSET + 0x3F000000) // corresponding to bus address 0x7E000000
#define MAILBOX_BASE    MMU_BASE + 0xb880


#endif