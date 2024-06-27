#include "tmpfs.h"
#include "vfs.h"
#include "string.h"
#include "memory.h"

struct file_operations tmpfs_file_operations = {
    .write = tmpfs_write,
    .read = tmpfs_read,
    .open = tmpfs_open,
    .close = tmpfs_close,
    .lseek64 = tmpfs_lseek64,
};

struct vnode_operations tmpfs_vnode_operations = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir,
};

int register_tmpfs()
{
  struct filesystem *fs = (struct filesystem *)kmalloc(sizeof(struct filesystem));
  fs->name = "tmpfs";
  fs->setup_mount = tmpfs_setup_mount;
  return register_filesystem(fs);
}

int tmpfs_write(struct file *file, const void *buf, unsigned int len)
{
  struct tmpfs_node *node = (struct tmpfs_node *)file->vnode->internal;

  if (file->f_pos + len > TMPFS_MAX_FILE_SIZE)
    len = TMPFS_MAX_FILE_SIZE - file->f_pos;

  memcpy((void *)(node->data + file->f_pos), (void *)buf, len);
  file->f_pos += len;
  if (node->size < file->f_pos)
    node->size = file->f_pos;

  return len;
}

int tmpfs_read(struct file *file, void *buf, unsigned int len)
{
  struct tmpfs_node *node = (struct tmpfs_node *)file->vnode->internal;

  if (file->f_pos + len > node->size)
    len = node->size - file->f_pos;

  memcpy(buf, node->data + file->f_pos, len);
  file->f_pos += len;

  return len;
}

int tmpfs_open(struct vnode *node, struct file **file)
{
  (*file)->vnode = node;
  (*file)->f_pos = 0;
  (*file)->f_ops = node->f_ops;
  return 0;
}

int tmpfs_close(struct file *file)
{
  kfree(file);
  return 0;
}

long tmpfs_lseek64(struct file *file, long offset, int whence)
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

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
  struct tmpfs_node *dir = (struct tmpfs_node *)dir_node->internal;
  for (int i = 0; i < TMPFS_MAX_ENTRIES; i++)
  {
    if (dir->entries[i] != 0 && strcmp(((struct tmpfs_node *)dir->entries[i]->internal)->name, component_name) == 0)
    {
      *target = dir->entries[i];
      return 0;
    }
  }
  return -1;
}

int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
  struct tmpfs_node *dir = (struct tmpfs_node *)dir_node->internal;
  for (int i = 0; i < TMPFS_MAX_ENTRIES; i++)
  {
    if (dir->entries[i] == 0)
    {
      struct tmpfs_node *new_node = (struct tmpfs_node *)kmalloc(sizeof(struct tmpfs_node));
      strcpy(new_node->name, component_name);
      new_node->is_dir = 0;
      new_node->size = 0;
      new_node->data = (char *)kmalloc(TMPFS_MAX_FILE_SIZE);
      memset(new_node->data, 0, TMPFS_MAX_FILE_SIZE);
      for (int j = 0; j < TMPFS_MAX_ENTRIES; j++)
        new_node->entries[j] = 0;

      *target = kmalloc(sizeof(struct vnode));
      (*target)->parent = dir_node;
      (*target)->internal = (void *)new_node;
      (*target)->f_ops = dir_node->f_ops;
      (*target)->v_ops = dir_node->v_ops;
      (*target)->mount = 0;

      dir->entries[i] = *target;
      return 0;
    }
  }
  return -1;
}

int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
  struct tmpfs_node *dir = (struct tmpfs_node *)dir_node->internal;
  for (int i = 0; i < TMPFS_MAX_ENTRIES; i++)
  {
    if (dir->entries[i] == 0)
    {
      struct tmpfs_node *new_node = (struct tmpfs_node *)kmalloc(sizeof(struct tmpfs_node));
      strcpy(new_node->name, component_name);
      new_node->is_dir = 1;
      new_node->size = 0;
      new_node->data = 0;
      for (int j = 0; j < TMPFS_MAX_ENTRIES; j++)
        new_node->entries[j] = 0;

      *target = (struct vnode *)kmalloc(sizeof(struct vnode));
      (*target)->parent = dir_node;
      (*target)->internal = (void *)new_node;
      (*target)->f_ops = dir_node->f_ops;
      (*target)->v_ops = dir_node->v_ops;
      (*target)->mount = 0;
      dir->entries[i] = *target;
      return 0;
    }
  }
  return -1;
}

int tmpfs_setup_mount(struct filesystem *fs, struct mount *mount)
{
  struct tmpfs_node *root = (struct tmpfs_node *)kmalloc(sizeof(struct tmpfs_node));
  root->is_dir = 1;
  root->size = 0;
  root->data = 0;
  for (int i = 0; i < TMPFS_MAX_ENTRIES; i++)
    root->entries[i] = 0;

  mount->root = (struct vnode *)kmalloc(sizeof(struct vnode));
  mount->fs = fs;
  mount->root->v_ops = &tmpfs_vnode_operations;
  mount->root->f_ops = &tmpfs_file_operations;
  mount->root->mount = 0;
  mount->root->internal = (void *)root;
  return 0;
}
