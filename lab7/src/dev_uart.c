#include "dev_uart.h"
#include "uart.h"

struct file_operations dev_uart_fops = {
    .open = dev_uart_open,
    .close = dev_uart_close,
    .read = dev_uart_read,
    .write = dev_uart_write,
    .lseek64 = dev_uart_lseek64,
};

int dev_uart_register()
{
    return register_device(&dev_uart_fops);
}

int dev_uart_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_uart_fops;
    return 0;
}

int dev_uart_close(struct file *file)
{
    // kfree(file);
    return 0;
}

int dev_uart_read(struct file *file, void *buf, size_t len)
{
    char *b = buf;
    for (int i = 0; i < len; i++)
        b[i] = uart_getc();
    return len;
}

int dev_uart_write(struct file *file, const void *buf, size_t len)
{
    const char *b = buf;
    for (int i = 0; i < len; i++)
        uart_putc(b[i]);
    return len;
}

long dev_uart_lseek64(struct file *file, long offset, int whence)
{
    return -1;
}
