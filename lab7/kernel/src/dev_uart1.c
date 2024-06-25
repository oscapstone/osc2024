#include "vfs.h"
#include "dev_uart1.h"
#include "mini_uart.h"
#include "memory.h"

const struct file_operations dev_file_operations = {dev_uart1_async_write, dev_uart1_async_read, dev_uart1_open, dev_uart1_close, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny};
const struct file_operations stdin_ops = {(void *)dev_uart1_op_deny, dev_uart1_async_read, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny};
const struct file_operations stdout_ops = {dev_uart1_async_write, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny};
const struct file_operations stderr_ops = {dev_uart1_write, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny, (void *)dev_uart1_op_deny};
int uart1_dev_id = -1;

int init_dev_uart1()
{
	dev_t *dev = kmalloc(sizeof(dev_t));
	dev->name = "uart1";
	uart_puts("uart1->f_ops: 0x%x\r\n", dev->f_ops);
	dev->f_ops = &dev_file_operations;
	uart_puts("uart1->f_ops: 0x%x\r\n", dev->f_ops);
	uart1_dev_id = register_dev(dev);
	return 0;
}

int dev_uart1_write(struct file *file, const void *buf, size_t len)
{
	// uart_puts("dev_uart1_async_write: %s, len: %d\r\n", buf, len);
	const char *cbuf = buf;
	for (int i = 0; i < len; i++)
	{
		uart_send(cbuf[i]);
	}
	return len;
}

int dev_uart1_async_write(struct file *file, const void *buf, size_t len)
{
	// uart_puts("dev_uart1_async_write: %s, len: %d\r\n", buf, len);
	const char *cbuf = buf;
	for (int i = 0; i < len; i++)
	{
		uart_async_send(cbuf[i]);
	}
	return len;
}

int dev_uart1_async_read(struct file *file, void *buf, size_t len)
{
	char *cbuf = buf;
	for (int i = 0; i < len; i++)
	{
		cbuf[i] = uart_async_getc();
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

struct file_operations *get_stdin_ops()
{
	return &stdin_ops;
}

struct file_operations *get_stdout_ops()
{
	return &stdout_ops;
}

struct file_operations *get_stderr_ops()
{
	return &stderr_ops;
}