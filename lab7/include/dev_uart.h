#ifndef DEV_UART_H
#define DEV_UART_H

#include "vfs.h"

int dev_uart_register();
int dev_uart_open(struct vnode *file_node, struct file **target);
int dev_uart_close(struct file *file);
int dev_uart_read(struct file *file, void *buf, size_t len);
int dev_uart_write(struct file *file, const void *buf, size_t len);
long dev_uart_lseek64(struct file *file, long offset, int whence);

#endif // DEV_UART_H
