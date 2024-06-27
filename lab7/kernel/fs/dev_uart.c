#include <kernel/fs/vfs.h>
#include <kernel/fs/dev_uart.h>
#include <bsp/uart.h>
#include <kernel/memory.h>
#include <kernel/io.h>

struct file_operations dev_uart_operations = {dev_uart_write, dev_uart_read, dev_uart_open, dev_uart_close, (void *)dev_uart_deny, (void *)dev_uart_deny};

int init_dev_uart()
{
    return register_dev(&dev_uart_operations);
}

int dev_uart_write(struct file *file, const void *buf, size_t len)
{
    /*printf("dev_uart_write\n");*/
    const char *cbuf = buf;
    for (int i = 0; i < len; i++)
    {
        uart_send(cbuf[i]);
    }
    return len;
}

int dev_uart_read(struct file *file, void *buf, size_t len)
{
    char *cbuf = buf;
    for (int i = 0; i < len; i++)
    {
        cbuf[i] = uart_recv();
    }
    return len;
}

int dev_uart_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_uart_operations;
    return 0;
}

int dev_uart_close(struct file *file)
{
    kfree(file);
    return 0;
}

int dev_uart_deny()
{
    return -1;
}
