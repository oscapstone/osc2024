#include "dev_uart.h"
#include "mini_uart.h"
#include "vfs.h"
#include "memory.h"

struct file_operations dev_uart_ops = {
    .write = dev_uart_write,
    .read = dev_uart_read,
    .open = dev_uart_open,
    .close = dev_uart_close,
    .lseek64 = dev_uart_lseek64,
};

int dev_uart_write(struct file *file, const void *buf, unsigned int len)
{
  for (int i = 0; i < len; i++)
  {
    uart_sendc(*((char *)buf + i));
  }
  return len;
}

int dev_uart_read(struct file *file, void *buf, unsigned int len)
{
  for (int i = 0; i < len; i++)
  {
    ((char *)buf)[i] = uart_recv();
  }
  return len;
}

int dev_uart_open(struct vnode *node, struct file **file)
{
  (*file)->vnode = node;
  (*file)->f_pos = 0;
  (*file)->f_ops = &dev_uart_ops;
  return 0;
}

int dev_uart_close(struct file *file)
{
  kfree(file);
  return 0;
}

long dev_uart_lseek64(struct file *file, long offset, int whence)
{
  long new_pos = 0;
  switch (whence)
  {
  case SEEK_SET:
    new_pos = offset;
    break;
  default:
    return -1;
  }

  if (new_pos < 0)
    return -1;

  file->f_pos = new_pos;
  return new_pos;
}
