#ifndef __DEV_UART_H__
#define __DEV_UART_H__

#include "vfs.h"

int dev_uart_register();
int dev_uart_write(struct file *file, const void *buf, size_t len);
int dev_uart_read(struct file *file, void *buf, size_t len);
int dev_uart_open(struct vnode *file_node, struct file **target);
int dev_uart_close(struct file *file);

#endif