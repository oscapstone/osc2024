#include "vfs.h"
#include "dev_uart1.h"
#include "uart1.h"
#include "memory.h"
 
struct file_operations dev_file_operations = {dev_uart1_write, dev_uart1_read, dev_uart1_open, dev_uart1_close, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny};
int uart1_dev_id = -1;

int init_dev_uart1()
{
	dev_t *dev = kmalloc(sizeof(dev_t));
	dev->name = "uart1";
	dev->f_ops = &dev_file_operations;
	uart1_dev_id = register_dev(dev);
    return 0;
}

int dev_uart1_write(struct file *file, const void *buf, size_t len)
{
    const char *cbuf = buf;
    for (int i = 0; i < len;i++)
    {
        uart_async_send(cbuf[i]);
    }
    return len;
}

int dev_uart1_read(struct file *file, void *buf, size_t len)
{
    char *cbuf = buf;
    for (int i = 0; i < len; i++)
    {
        cbuf[i] = uart_async_recv();
    }
    return len;
}

int dev_uart1_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_file_operations;
    return 0;
}

int dev_uart1_close(struct file *file)
{
    kfree(file);
    return 0;
}

int dev_uart1_op_deny()
{
    return -1;
}
