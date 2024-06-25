#ifndef _RPI_MBOX_H_
#define _RPI_MBOX_H_

#include "bcm2837/rpi_base.h"

#define MBOX_BASE     (PERIPHERAL_BASE+0x0000B880)

// The register access to a mailbox
// https://jsandler18.github.io/extra/mailbox.html
#define MBOX_READ     ((volatile unsigned int*)(MBOX_BASE+0x00))
#define MBOX_POLL     ((volatile unsigned int*)(MBOX_BASE+0x10))
#define MBOX_SENDER   ((volatile unsigned int*)(MBOX_BASE+0x14))
#define MBOX_STATUS   ((volatile unsigned int*)(MBOX_BASE+0x18))
#define MBOX_CONFIG   ((volatile unsigned int*)(MBOX_BASE+0x1C))
#define MBOX_WRITE    ((volatile unsigned int*)(MBOX_BASE+0x20))

#endif  /*_RPI_MBOX_H_ */
