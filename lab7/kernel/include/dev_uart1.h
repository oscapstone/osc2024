#ifndef _DEV_UART1_H_
#define _DEV_UART1_H_

#include "stddef.h"
#include "vfs.h"

int init_dev_uart1();

int dev_uart1_write(struct file *file, const void *buf, size_t len);
int dev_uart1_async_write(struct file *file, const void *buf, size_t len);
int dev_uart1_async_read(struct file *file, void *buf, size_t len);
int dev_uart1_open(struct vnode *file_node, struct file **target);
int dev_uart1_close(struct file *file);
int dev_uart1_op_deny();
struct file_operations* get_stdin_ops();
struct file_operations* get_stdout_ops();
struct file_operations *get_stderr_ops();

#endif /* _DEV_UART1_H_ */