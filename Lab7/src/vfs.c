#include "vfs.h"
#include "mini_uart.h"
#include "string.h"
#include "memory.h"
#include "tmpfs.h"
#include "initramfs.h"
#include "thread.h"
#include "dev_framebuffer.h"

extern struct file_operations dev_uart_ops;
extern struct file_operations dev_framebuffer_ops;

struct mount *rootfs;

struct filesystem *fs_list[10] = {0};

int isFsInitialized;

int register_filesystem(struct filesystem *fs)
{
  for (int i = 0; i < 10; i++)
  {
    if (fs_list[i] == 0)
    {
      fs_list[i] = fs;
      return 0;
    }
  }
  uart_sends("Too many filesystems\n");
  return -1;
}

int vfs_open(const char *pathname, int flags, struct file **target)
{
  struct vnode *node;
  int ret = vfs_lookup(pathname, &node);

  if (ret)
  { // file not found
    if (flags & O_CREAT)
    { // create a new file
      char *path = strdup(pathname);
      int len = strlen(pathname), index = 0;
      for (int i = 0; i < len; i++)
      {
        if (pathname[i] == '/')
        {
          index = i;
        }
      }
      if (index == 0)
      {
        path[1] = '\0';
      }
      else
        path[index] = '\0';
      struct vnode *dir_node;
      ret = vfs_lookup(path, &dir_node);
      if (ret)
      {
        uart_sends("open: No such file or directory\n");
        return -1;
      }
      dir_node->v_ops->create(dir_node, &node, pathname + index + 1);
    }
    else
    {
      uart_sends("File not found\n");
      return -1;
    }
  }
  *target = (struct file *)kmalloc(sizeof(struct file));
  ret = node->f_ops->open(node, target);
  (*target)->flags = flags;
  return ret;
}

int vfs_close(struct file *file)
{
  int ret = file->f_ops->close(file);
  return ret;
}

int vfs_lseek64(struct file *file, long offset, int whence)
{
  return file->f_ops->lseek64(file, offset, whence);
}

int vfs_write(struct file *file, const void *buf, unsigned int len)
{
  return file->f_ops->write(file, buf, len);
}

int vfs_read(struct file *file, void *buf, unsigned int len)
{
  return file->f_ops->read(file, buf, len);
}

int vfs_mkdir(const char *pathname)
{
  struct vnode *node;
  int ret = vfs_lookup(pathname, &node);
  if (ret == 0)
  {
    uart_sends("Directory already exists\n");
    return -1;
  }
  char *path = strdup(pathname);
  int len = strlen(pathname), index = 0;
  for (int i = 0; i < len; i++)
  {
    if (pathname[i] == '/')
    {
      index = i;
    }
  }
  if (index == 0)
  {
    path[1] = '\0';
  }
  else
    path[index] = '\0';
  struct vnode *dir_node;
  ret = vfs_lookup(path, &dir_node);
  if (ret)
  {
    uart_sends("mkdir: No such file or directory\n");
    return -1;
  }
  dir_node->v_ops->mkdir(dir_node, &node, pathname + index + 1);
  return 0;
}

int vfs_mknod(const char *pathname, struct file_operations *f_ops)
{
  struct file *file;
  vfs_open(pathname, O_CREAT, &file);
  file->vnode->f_ops = f_ops;
  vfs_close(file);
  return 0;
}

int vfs_mount(const char *target, const char *filesystem)
{
  struct mount *mount = (struct mount *)kmalloc(sizeof(struct mount));
  for (int i = 0; i < 10; i++)
  {
    if (fs_list[i] != 0)
    {
      if (strcmp(fs_list[i]->name, filesystem) == 0)
      {
        struct vnode *node;
        vfs_lookup(target, &node);
        node->mount = mount;
        fs_list[i]->setup_mount(fs_list[i], mount);
        mount->root->parent = node->parent;
        return 0;
      }
    }
  }
  uart_sends("Filesystem not found\n");
  return -1;
}

int vfs_lookup(const char *pathname, struct vnode **target)
{
  if (strcmp(pathname, "/") == 0)
  {
    *target = rootfs->root;
    return 0;
  }

  struct vnode *node;
  if (isFsInitialized)
  {
    thread *current = get_current();
    node = current->cwd;
  }
  else
  {
    node = rootfs->root;
  }

  char *path = strdup(pathname);
  char *token;
  if (path[0] == '/')
    token = strtok(path + 1, "/");
  else
    token = strtok(path, "/");

  while (token != 0)
  {
    if (strcmp(token, "..") == 0)
    {
      if (node->parent != 0)
      {
        node = node->parent;
      }
    }
    else if (strcmp(token, ".") == 0)
    {
    }
    else
    {
      struct vnode *next_node;
      int ret = node->v_ops->lookup(node, &next_node, token);
      if (ret)
      {
        // uart_sends("File not found\n");
        return -1;
      }
      while (next_node->mount)
        next_node = next_node->mount->root;
      node = next_node;
    }
    token = strtok(0, "/");
  }
  *target = node;
  return 0;
}

int vfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
  return dir_node->v_ops->create(dir_node, target, component_name);
}

int init_vfs()
{
  isFsInitialized = 0;
  register_tmpfs();
  rootfs = (struct mount *)kmalloc(sizeof(struct mount));
  fs_list[0]->setup_mount(fs_list[0], rootfs);
  rootfs->root->parent = 0;

  vfs_mkdir("/initramfs");
  register_initramfs();
  vfs_mount("/initramfs", "initramfs");
  struct vnode *initramfs_node;
  vfs_lookup("/initramfs", &initramfs_node);
  initramfs_init(initramfs_node);

  vfs_mkdir("/dev");
  vfs_mknod("/dev/uart", &dev_uart_ops);

  vfs_mknod("/dev/framebuffer", &dev_framebuffer_ops);
  framebuffer_init();
  // isFsInitialized = 1;
  return 0;
}
