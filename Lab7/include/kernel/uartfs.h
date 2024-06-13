#ifndef UARTFS_H
#define UARTFS_H

#include "kernel/vfs.h"

int init_dev_uart(void);

int dev_uart_write(struct file *file, const void *buf, my_uint64_t len);
int dev_uart_read(struct file *file, void *buf, my_uint64_t len);
int dev_uart_open(struct vnode *file_node, struct file **target);
int dev_uart_close(struct file *file);

#endif