#ifndef _DEF_INITRAMFS
#define _DEF_INITRAMFS

#include "cpio.h"
#include "fdt.h"

void parse_initramfs(int);
void list_initramfs(void);
cpio_meta_t *find_initramfs(const char *);
void cat_initramfs(void);

int initramfs_callback(int);

#endif
