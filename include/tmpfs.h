#ifndef _TMPFS_H
#define _TMPFS_H

#include "vfs.h"

#define TMPFS_MAX_COMPONENT_NAME_LEN 5
#define TMPFS_MAX_ENTRY_N 16
#define TMPFS_MAX_INTERNAL_SZ 4096

typedef struct tmpfs_internal {
  char data[TMPFS_MAX_INTERNAL_SZ];
} tmpfs_internal;

filesystem* tmpfs_init();

#endif