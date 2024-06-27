#ifndef __DEV_UART_H__
#define __DEV_UART_H__

#include "vfs.h"
int dev_uart_write(struct file *file, const void *buf, unsigned int len);
int dev_uart_read(struct file *file, void *buf, unsigned int len);
int dev_uart_open(struct vnode *node, struct file **file);
int dev_uart_close(struct file *file);
long dev_uart_lseek64(struct file *file, long offset, int whence);

#endif