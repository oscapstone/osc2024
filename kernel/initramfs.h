#ifndef _DEF_INITRAMFS
#define _DEF_INITRAMFS

#include "fdt.h"

void parse_initramfs(int);
void list_initramfs(void);
void cat_initramfs(void);

// TODO: callback 直接改成 parse_initramfs?
// 因為 traverse 完 dtb 應該能得到 initramfs 的位址，就不用 hard code
int initramfs_callback(int);    // 可以用 fdt_callback_t 而非 int 嗎？


#endif
