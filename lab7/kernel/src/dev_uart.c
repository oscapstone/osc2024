#include "vfs.h"
#include "dev_uart.h"
#include "uart1.h"
#include "memory.h"

struct file_operations dev_file_operations = {dev_uart_write, dev_uart_read, dev_uart_open, dev_uart_close, (void *)dev_uart_op_deny, (void *)dev_uart_op_deny};

// VFS為uart分配一個ID, VFS就可辨識device
// 初始化UART並註冊到 VFS
int init_dev_uart()
{
    return register_dev(&dev_file_operations);
}

// 將data寫入UART
int dev_uart_write(struct file *file, const void *buf, size_t len)
{
    const char *cbuf = buf;
    for (int i = 0; i < len;i++)
    {
        uart_async_putc(cbuf[i]);
    }
    return len;
}

// 從UART讀取data
int dev_uart_read(struct file *file, void *buf, size_t len)
{
    char *cbuf = buf;
    for (int i = 0; i < len; i++)
    {
        cbuf[i] = uart_async_getc();
    }
    return len;
}

// 打開 UART file，初始化file descriptors table
int dev_uart_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = &dev_file_operations;
    return 0;
}

// 關閉UART file，release資源
int dev_uart_close(struct file *file)
{
    kfree(file);
    return 0;
}

int dev_uart_op_deny()
{
    return -1;
}
