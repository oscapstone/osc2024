#include "dev_uart.h"
#include "mini_uart.h"
#include "alloc.h"

struct file_operations dev_file_operations = {
    dev_uart_write,
    dev_uart_read,
    dev_uart_open,
    dev_uart_close,
    0 // getsizeq
};


int dev_uart_register()
{
    uart_send_string("\r\n[INFO] Registering uart device...");
    return register_dev(&dev_file_operations);
}

int dev_uart_write(struct file *file, const void *buf, size_t len)
{
    uart_send_string("\r\n[INFO] Writing to console...");
    const char *cbuf = buf;
    for (int i = 0; i < len;i++)
    {
        uart_send(cbuf[i]);
    }
    return len;
}

int dev_uart_read(struct file *file, void *buf, size_t len)
{
    uart_send_string("\r\n[INFO] Reading from uart...");
    for (int i = 0; i < len; i++)
    {
        ((char*)buf)[i] = uart_recv();
    }
    return len;
}

int dev_uart_open(struct vnode *file_node, struct file **target)
{
    uart_send_string("\r\n[INFO] Opening uart device...");
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_file_operations;
    return 0;
}

int dev_uart_close(struct file *file)
{
    dfree(file);
    return 0;
}

int dev_uart_op_deny()
{
    return -1;
}
