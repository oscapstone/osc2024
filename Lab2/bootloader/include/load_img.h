#ifndef LOAD_IMG_H
#define LOAD_IMG_H

#define KERNEL_LOAD_ADDR 0x80000

#define KERNEL_LOAD_SUCCESS  0
#define KERNEL_LOAD_ERROR    1
#define KERNEL_SIZE_ERROR    2

int load_img(void);

#endif /* LOAD_IMG_H */
