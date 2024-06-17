#include "vfs.h"
#include "vfs_dev_uart.h"
#include "uart1.h"
#include "memory.h"
#include "string.h"

struct file_operations dev_file_operations = {dev_uart_write, dev_uart_read, dev_uart_open, dev_uart_close, (void *)dev_uart_op_deny, (void *)dev_uart_op_deny};

int init_dev_uart()
{
    return register_dev(&dev_file_operations);
}

int dev_uart_write(struct file *file, const void *buf, size_t len)
{
    char *cbuf = buf;
    int i = len;
    while (i--)
    {
        uart_async_send(*(cbuf++));
    }
    return len;
}

int dev_uart_read(struct file *file, void *buf, size_t len)
{
    char *cbuf = buf;
    int i = len;
    while (i--)
    {
        *cbuf = uart_async_recv();
        cbuf++;
    }
    return len;
}

int dev_uart_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_file_operations;
    return 0;
}

int dev_uart_close(struct file *file)
{
    kfree(file);
    return 0;
}

int dev_uart_op_deny()
{
    return -1;
}
