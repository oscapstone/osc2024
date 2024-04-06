#ifndef _DEF_INITRAMFS
#define _DEF_INITRAMFS

#include "fdt.h"

void parse_initramfs(int);
void list_initramfs(void);
void cat_initramfs(void);

int initramfs_callback(int);

#endif
