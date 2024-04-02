#ifndef _DEF_INITRAMFS
#define _DEF_INITRAMFS

#define PADDING_4(var) ((var) + 3 & (~0x03))

void parse_initramfs(void);
void list_initramfs(void);
void cat_initramfs(void);

#endif
