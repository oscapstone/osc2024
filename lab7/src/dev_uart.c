#include "dev_uart.h"
#include "memory.h"
#include "uart1.h"
#include "vfs.h"

struct file_operations dev_file_ops = {
    .open = dev_uart_open,
    .close = dev_uart_close,
    .read = dev_uart_read,
    .write = dev_uart_write,
    .lseek64 = (void *)op_deny,
    .getsize = (void *)op_deny,
};

int init_dev_uart() { return register_dev(&dev_file_ops); }

int dev_uart_write(struct file *file, const void *buf, size_t len)
{
    const char *cbuf = buf;
    for (int i = 0; i < len; i++)
        // uart_send(cbuf[i]);
        uart_async_putc(cbuf[i]);

    return len;
}

int dev_uart_read(struct file *file, void *buf, size_t len)
{
    char *cbuf = buf;
    for (int i = 0; i < len; i++)
        // cbuf[i] = uart_recv();
        cbuf[i] = uart_async_getc();

    return len;
}

int dev_uart_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_file_ops;
    return 0;
}

int dev_uart_close(struct file *file)
{
    kfree(file);
    return 0;
}

int op_deny() { return -1; }