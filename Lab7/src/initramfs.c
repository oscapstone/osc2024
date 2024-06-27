#include "initramfs.h"
#include "vfs.h"
#include "initrd.h"
#include "string.h"
#include "memory.h"

struct vnode_operations initramfs_v_ops = {
    .lookup = initramfs_lookup,
    .create = initramfs_create,
    .mkdir = initramfs_mkdir,
};

struct file_operations initramfs_f_ops = {
    .write = initramfs_write,
    .read = initramfs_read,
    .open = initramfs_open,
    .close = initramfs_close,
    .lseek64 = initramfs_lseek64,
};

int register_initramfs()
{
  struct filesystem *fs = (struct filesystem *)kmalloc(sizeof(struct filesystem));
  fs->name = "initramfs";
  fs->setup_mount = initramfs_setup_mount;
  return register_filesystem(fs);
}

int initramfs_init(struct vnode *root_node)
{
  char *current = (char *)get_initrd_start();
  cpio_header *head;

  int entry_count = 0;
  while (!memcmp(current, "070701", 6) && memcmp(current + sizeof(cpio_header), "TRAILER!!!", 10))
  {
    char filename[256];
    head = (cpio_header *)current;
    int filesize = hstr2int(head->c_filesize, 8);
    int namesize = hstr2int(head->c_namesize, 8);
    int n_padding = (4 - ((namesize + sizeof(cpio_header)) % 4)) % 4;
    int f_padding = (4 - filesize % 4) % 4;

    for (int i = 0; i < namesize; i++)
    {
      filename[i] = *(current + sizeof(cpio_header) + i);
    }
    filename[namesize] = '\0';
    struct initramfs_node *initrd_node = (struct initramfs_node *)kmalloc(sizeof(struct initramfs_node));
    initrd_node->name = strdup(filename);
    // strcpy(initrd_node->name, filename);
    initrd_node->is_dir = 0;
    initrd_node->size = filesize;
    initrd_node->data = kmalloc(filesize);
    memcpy(initrd_node->data, (current + sizeof(cpio_header) + namesize + n_padding), filesize);

    struct vnode *node = (struct vnode *)kmalloc(sizeof(struct vnode));
    node->v_ops = &initramfs_v_ops;
    node->f_ops = &initramfs_f_ops;
    node->mount = 0;
    node->parent = root_node;
    node->internal = (void *)initrd_node;

    ((struct initramfs_node *)root_node->internal)->entries[entry_count++] = node;
    current += (filesize + namesize + sizeof(cpio_header) + n_padding + f_padding);
  }
  return 0;
}

int initramfs_write(struct file *file, const void *buf, unsigned int len)
{
  return -1;
}

int initramfs_read(struct file *file, void *buf, unsigned int len)
{
  struct initramfs_node *node = (struct initramfs_node *)file->vnode->internal;

  if (file->f_pos + len > node->size)
    len = node->size - file->f_pos;

  memcpy(buf, node->data + file->f_pos, len);
  file->f_pos += len;

  return len;
}

int initramfs_open(struct vnode *node, struct file **file)
{
  (*file)->vnode = node;
  (*file)->f_pos = 0;
  (*file)->f_ops = node->f_ops;
  return 0;
}

int initramfs_close(struct file *file)
{
  kfree(file);
  return 0;
}

long initramfs_lseek64(struct file *file, long offset, int whence)
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

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
  struct initramfs_node *dir = (struct initramfs_node *)dir_node->internal;
  for (int i = 0; i < INITRAMFS_MAX_ENTRIES; i++)
  {
    if (dir->entries[i] != 0 && strcmp(((struct initramfs_node *)dir->entries[i]->internal)->name, component_name) == 0)
    {
      *target = dir->entries[i];
      return 0;
    }
  }
  return -1;
}

int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
  return -1;
}

int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
  return -1;
}

int initramfs_setup_mount(struct filesystem *fs, struct mount *mount)
{
  struct initramfs_node *root = (struct initramfs_node *)kmalloc(sizeof(struct initramfs_node));
  root->is_dir = 1;
  root->size = 0;
  root->data = 0;
  for (int i = 0; i < INITRAMFS_MAX_ENTRIES; i++)
    root->entries[i] = 0;

  mount->root = (struct vnode *)kmalloc(sizeof(struct vnode));
  mount->fs = fs;
  mount->root->v_ops = &initramfs_v_ops;
  mount->root->f_ops = &initramfs_f_ops;
  mount->root->mount = 0;
  mount->root->internal = (void *)root;
  return 0;
}
