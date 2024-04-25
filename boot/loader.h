#ifndef _DEF_LOADER
#define _DEF_LOADER

void load_kernel(void);
int get_kernel_size(const char*, int);
#define KERNEL_BASE ((volatile char *)(0x80000))

#endif
