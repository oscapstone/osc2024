#ifndef _DEV_UART_H_
#define _DEV_UART_H_

#include "stddef.h"
#include "vfs.h"

int init_dev_uart();

int dev_uart_write(struct file *file, const void *buf, size_t len);
int dev_uart_read(struct file *file, void *buf, size_t len);
int dev_uart_open(struct vnode *file_node, struct file **target);
int dev_uart_close(struct file *file);
int op_deny();
#endif