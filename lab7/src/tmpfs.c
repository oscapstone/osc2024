#include "tmpfs.h"

#include "mem.h"
#include "str.h"
#include "uart.h"
#include "utils.h"
#include "vfs.h"

file_operations tmpfs_file_operations = {
    tmpfs_write,   //
    tmpfs_read,    //
    tmpfs_open,    //
    tmpfs_close,   //
    tmpfs_lseek64, //
    tmpfs_getsize  //
};

vnode_operations tmpfs_vnode_operations = {
    tmpfs_lookup, //
    tmpfs_create, //
    tmpfs_mkdir   //
};

int tmpfs_setup_mount(filesystem *fs, mount *mnt) {
  mnt->fs = fs;
  mnt->root = tmpfs_create_vnode(dir_t);
  return 0;
}

vnode *tmpfs_create_vnode(fsnode_type type) {
  vnode *v = kmalloc(sizeof(vnode), SILENT);
  v->f_ops = &tmpfs_file_operations;
  v->v_ops = &tmpfs_vnode_operations;
  v->mount = 0;

  tmpfs_inode *inode = kmalloc(sizeof(tmpfs_inode), SILENT);
  memset(inode, 0, sizeof(tmpfs_inode));
  inode->type = type;
  inode->data = kmalloc(PAGE_SIZE, SILENT);
  v->internal = inode;

  return v;
}

int tmpfs_write(file *file, const void *buf, size_t len) {
  tmpfs_inode *inode = file->vnode->internal;
  memcpy(inode->data + file->f_pos, buf, len);
  file->f_pos += len;
  if (inode->datasize < file->f_pos)
    inode->datasize = file->f_pos;
  return len;
}

int tmpfs_read(file *file, void *buf, size_t len) {
  tmpfs_inode *inode = file->vnode->internal;
  if (len + file->f_pos > inode->datasize) {
    len = inode->datasize - file->f_pos;
  }

  strncpy(buf, inode->data + file->f_pos, len);
  file->f_pos += inode->datasize - file->f_pos;
  return len;
}

int tmpfs_open(vnode *file_node, file *target) {
  target->vnode = file_node;
  target->f_ops = file_node->f_ops;
  target->f_pos = 0;
  return 0;
}

int tmpfs_close(file *f) {
  kfree(f, SILENT);
  return 0;
}

// long seek 64-bit
long tmpfs_lseek64(file *f, long offset, int whence) {
  // uart_log(DEBUG, "tmpfs_lseek64\n");
  if (whence == SEEK_SET) {
    // return current position as offset
    f->f_pos = offset;
  } else if (whence == SEEK_CUR) {
    // return current position + offset
    long size = f->f_ops->getsize(f->vnode);
    f->f_pos += offset;
    if (f->f_pos > size)
      f->f_pos = size;
  } else
    return not_supported();
  return f->f_pos;
}

long tmpfs_getsize(vnode *vn) {
  tmpfs_inode *inode = vn->internal;
  return inode->datasize;
}

int tmpfs_lookup(vnode *dir_node, vnode **target, const char *component_name) {
  tmpfs_inode *dir_inode = dir_node->internal;
  int child_idx = 0;

  // BFS search tree
  for (; child_idx <= MAX_DIR_ENTRY; child_idx++) {
    vnode *vnode_it =
        dir_inode->entry[child_idx]; // entry stores all sub folder/file
    if (!vnode_it)
      break;
    tmpfs_inode *inode_it = vnode_it->internal;
    if (strcmp(component_name, inode_it->name) == 0) {
      *target = vnode_it;
      return 0;
    }
  }
  return -1;
}

int tmpfs_create(vnode *dir_node, vnode **target, const char *component_name) {
  tmpfs_inode *inode = dir_node->internal;
  if (inode->type != dir_t) {
    uart_puts("[tmpfs create] not a dir\n");
    return -1;
  }

  int child_idx = 0;
  for (; child_idx <= MAX_DIR_ENTRY; child_idx++) {
    if (!inode->entry[child_idx])
      break;
    tmpfs_inode *child_inode = inode->entry[child_idx]->internal;
    if (!strcmp(child_inode->name, component_name)) {
      uart_log(WARN, "tmpfs_create: file exists\n");
      return -1;
    }
  }

  vnode *vn = tmpfs_create_vnode(file_t);
  inode->entry[child_idx] = vn;

  tmpfs_inode *new_inode = vn->internal;
  new_inode->name = kmalloc(strlen(component_name) + 1, SILENT);
  strncpy(new_inode->name, component_name, strlen(component_name) + 1);

  *target = vn;
  return 0;
}

int tmpfs_mkdir(vnode *dir_node, vnode **target, const char *component_name) {
  tmpfs_inode *inode = dir_node->internal;
  if (inode->type != dir_t) {
    uart_puts("[tmpfs mkdir] not a directory\n");
    return -1;
  }

  int child_idx = 0;
  for (; child_idx < MAX_DIR_ENTRY; child_idx++) {
    if (!inode->entry[child_idx])
      break;
  }

  if (child_idx > MAX_DIR_ENTRY) {
    uart_puts("[tmpfs mkdir] dir entry full\n");
    return -1;
  }

  vnode *vn = tmpfs_create_vnode(dir_t);
  inode->entry[child_idx] = vn;

  tmpfs_inode *new_inode = vn->internal;
  new_inode->name = kmalloc(strlen(component_name) + 1, SILENT);
  strncpy(new_inode->name, component_name, strlen(component_name) + 1);

  *target = vn;
  return 0;
}