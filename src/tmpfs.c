#include "tmpfs.h"

#include "cpio_.h"
#include "mem.h"
#include "string.h"
#include "uart1.h"
#include "vm.h"

static filesystem* tmpfs = (filesystem*)0;
static vnode_operations* tmpfs_v_ops = (vnode_operations*)0;
static file_operations* tmpfs_f_ops = (file_operations*)0;

static file* tmpfs_create_fd(vnode* file_node) {
  file* fp = (file*)malloc(sizeof(file));
  if (!fp) {
    return (file*)0;
  }
  fp->vnode = file_node;
  fp->f_ops = file_node->f_ops;
  fp->f_pos = 0;
  return fp;
}

static vnode* tmpfs_create_vnode(vnode* parent, const char* name,
                                 uint8_t type) {
  vnode* tmpfs_vnode = (vnode*)malloc(sizeof(vnode));
  if (!tmpfs_vnode) {
    return (vnode*)0;
  }
  strcpy(tmpfs_vnode->name, name);
  tmpfs_vnode->parent = parent;
  if (parent) {
    for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
      if (parent->subdirs[i] == (vnode*)0) {
        parent->subdirs[i] = tmpfs_vnode;
        break;
      }
    }
  }
  for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
    tmpfs_vnode->subdirs[i] = (vnode*)0;
  }
  tmpfs_vnode->mountpoint = (mount*)0;
  tmpfs_vnode->v_ops = tmpfs_v_ops;
  tmpfs_vnode->f_ops = tmpfs_f_ops;
  tmpfs_vnode->type = type;
  if (type == REGULAR_FILE) {
    tmpfs_vnode->internal = (void*)malloc(sizeof(tmpfs_internal));
    tmpfs_internal* internal = (tmpfs_internal*)tmpfs_vnode->internal;
    memset(internal->data, EOF, TMPFS_MAX_INTERNAL_SZ);
  } else {
    tmpfs_vnode->internal = (void*)0;
  }
  return tmpfs_vnode;
}

static int32_t tmpfs_setup_mount(filesystem* fs, mount* mount,
                                 const char* name) {
  mount->fs = fs;
  mount->root = tmpfs_create_vnode((vnode*)0, name, DIRECTORY);
  if (!mount->root) {
    return -1;
  }
  mount->root->mountpoint = mount;
  return 0;
}

static int32_t tmpfs_vnode_lookup(vnode* dir_node, vnode** target,
                                  const char* component_name) {
  if (!strcmp(".", component_name)) {
    *target = dir_node;
    return 0;
  }
  if (!strcmp("..", component_name)) {
    *target = dir_node->parent;
    return 0;
  }
  for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
    if (dir_node->subdirs[i] &&
        !strcmp(dir_node->subdirs[i]->name, component_name)) {
      *target = dir_node->subdirs[i];
      return 0;
    }
  }
  return -1;
}

static int32_t tmpfs_vnode_create(vnode* dir_node, vnode** target,
                                  const char* component_name) {
  *target = tmpfs_create_vnode(dir_node, component_name, REGULAR_FILE);
  if (!*target) {
    return -1;
  }
  return 0;
}

static int32_t tmpfs_vnode_mkdir(vnode* dir_node, vnode** target,
                                 const char* component_name) {
  if (dir_node->type != DIRECTORY) {
    uart_puts("tmpfs_vnode_mkdir: target_node isn't a directory");
    return -1;
  }

  *target = tmpfs_create_vnode(dir_node, component_name, DIRECTORY);
  if (!*target) {
    uart_puts("tmpfs_vnode_mkdir: tmpfs_create_vnode fails");
    return -1;
  }
  return 0;
}

static int32_t tmpfs_file_open(vnode* file_node, file** fp) {
  *fp = tmpfs_create_fd(file_node);
  if (!(*fp)) {
    return -1;
  }
  return 0;
}

static int32_t tmpfs_file_close(file* fp) {
  free((void*)fp);
  return 0;
}

static int32_t tmpfs_file_read(file* fp, void* buf, uint32_t len) {
  tmpfs_internal* internal = fp->vnode->internal;
  char *src = internal->data, *dst = (char*)buf;
  uint32_t i;
  for (i = 0; i < len && src[fp->f_pos] != (uint8_t)EOF; i++, fp->f_pos++) {
    dst[i] = src[fp->f_pos];
  }
  dst[i] = '\0';
  return i;
}

static int32_t tmpfs_file_write(file* fp, const void* buf, uint32_t len) {
  tmpfs_internal* internal = fp->vnode->internal;
  char* dst = internal->data;
  const char* src = (const char*)buf;
  uint32_t i;
  for (i = 0; i < len && src[i] != '\0' && fp->f_pos < TMPFS_MAX_INTERNAL_SZ;
       i++, fp->f_pos++) {
    dst[fp->f_pos] = src[i];
  }
  dst[fp->f_pos] = EOF;
  return i;
}

static int32_t tmpfs_register_fs() {
  if (!tmpfs_v_ops) {
    tmpfs_v_ops = (vnode_operations*)malloc(sizeof(vnode_operations));
    if (!tmpfs_v_ops) {
      return -1;
    }
    tmpfs_v_ops->create =
        (vfs_create_fp_t)phy2vir((uint64_t)tmpfs_vnode_create);
    tmpfs_v_ops->lookup =
        (vfs_lookup_fp_t)phy2vir((uint64_t)tmpfs_vnode_lookup);
    tmpfs_v_ops->mkdir = (vfs_mkdir_fp_t)phy2vir((uint64_t)tmpfs_vnode_mkdir);
  }

  if (!tmpfs_f_ops) {
    tmpfs_f_ops = (file_operations*)malloc(sizeof(file_operations));
    if (!tmpfs_f_ops) {
      return -1;
    }
    tmpfs_f_ops->open = (vfs_open_fp_t)phy2vir((uint64_t)tmpfs_file_open);
    tmpfs_f_ops->close = (vfs_close_fp_t)phy2vir((uint64_t)tmpfs_file_close);
    tmpfs_f_ops->read = (vfs_read_fp_t)phy2vir((uint64_t)tmpfs_file_read);
    tmpfs_f_ops->write = (vfs_write_fp_t)phy2vir((uint64_t)tmpfs_file_write);
  }
  return 0;
}

filesystem* tmpfs_init() {
  if (!tmpfs) {  // if tmpfs has not been initialized
    tmpfs = (filesystem*)malloc(sizeof(filesystem));
    strcpy(tmpfs->name, "tmpfs");
    tmpfs->setup_mnt = (vfs_setup_mount)phy2vir((uint64_t)tmpfs_setup_mount);
    tmpfs->register_fs = (vfs_register_fs)phy2vir((uint64_t)tmpfs_register_fs);
  }

  return tmpfs;
}