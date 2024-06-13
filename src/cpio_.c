#include "cpio_.h"

#include "mem.h"
#include "string.h"
#include "uart1.h"
#include "utli.h"
#include "vfs.h"
#include "vm.h"
#include "vm_macro.h"

static filesystem *initramfs = (filesystem *)0;
static vnode_operations *initramfs_v_ops = (vnode_operations *)0;
static file_operations *initramfs_f_ops = (file_operations *)0;
void *cpio_start_addr;
void *cpio_end_addr;

static uint32_t cpio_atoi(const char *s, int32_t char_size) {
  uint32_t num = 0;
  for (int i = 0; i < char_size; i++) {
    num = num * 16;
    if (*s >= '0' && *s <= '9') {
      num += (*s - '0');
    } else if (*s >= 'A' && *s <= 'F') {
      num += (*s - 'A' + 10);
    } else if (*s >= 'a' && *s <= 'f') {
      num += (*s - 'a' + 10);
    }
    s++;
  }
  return num;
}

void cpio_ls() {
  void *addr = cpio_start_addr;
  while (strcmp((char *)(addr + sizeof(cpio_header)), "TRAILER!!!") != 0) {
    cpio_header *header = (cpio_header *)addr;
    uint32_t filename_size =
        cpio_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
    uint32_t headerPathname_size = sizeof(cpio_header) + filename_size;
    uint32_t file_size =
        cpio_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
    align_inplace(&headerPathname_size, 4);
    align_inplace(&file_size, 4);
    uart_send_string(addr + sizeof(cpio_header));
    uart_send_string("  ");
    uart_int(file_size);
    uart_puts("B");
    addr += (headerPathname_size + file_size);
  }
}

char *findFile(const char *name) {
  void *addr = cpio_start_addr;
  while (strcmp((char *)(addr + sizeof(cpio_header)), "TRAILER!!!") != 0) {
    if (!strcmp((char *)(addr + sizeof(cpio_header)), name)) {
      return addr;
    }
    cpio_header *header = (cpio_header *)addr;
    uint32_t pathname_size =
        cpio_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
    uint32_t file_size =
        cpio_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
    uint32_t headerPathname_size = sizeof(cpio_header) + pathname_size;
    align_inplace(&headerPathname_size, 4);
    align_inplace(&file_size, 4);
    addr += (headerPathname_size + file_size);
  }
  return (char *)0;
}

void cpio_cat(const char *filename) {
  char *file = findFile(filename);
  if (file) {
    cpio_header *header = (cpio_header *)file;
    uint32_t filename_size =
        cpio_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
    uint32_t headerPathname_size = sizeof(cpio_header) + filename_size;
    uint32_t file_size =
        cpio_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
    align_inplace(&headerPathname_size, 4);
    align_inplace(&file_size, 4);
    char *file_content = (char *)header + headerPathname_size;
    for (uint32_t i = 0; i < file_size; i++) {
      uart_write(file_content[i]);
    }
    uart_send_string("\r\n");
  } else {
    uart_send_string(filename);
    uart_puts(" not found");
  }
}

char *cpio_load(const char *filename, uint32_t *file_sz) {
  char *file = (void *)phy2vir((uint64_t)findFile(filename));
  if (file) {
    cpio_header *header = (cpio_header *)file;
    uint32_t filename_size =
        cpio_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
    uint32_t headerPathname_size = sizeof(cpio_header) + filename_size;
    *file_sz = cpio_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
    align_inplace(&headerPathname_size, 4);
    align_inplace(file_sz, 4);
    char *file_content = (char *)header + headerPathname_size;
    return file_content;
  }
  *file_sz = 0;
  uart_send_string(filename);
  uart_puts(" not found");
  return (char *)0;
}

static vnode *initramfs_create_vnode(vnode *parent, const char *name,
                                     uint8_t type) {
  vnode *initramfs_vnode = (vnode *)malloc(sizeof(vnode));
  if (!initramfs_vnode) {
    return (vnode *)0;
  }
  strcpy(initramfs_vnode->name, name);
  initramfs_vnode->parent = parent;
  if (parent) {
    for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
      if (parent->subdirs[i] == (vnode *)0) {
        parent->subdirs[i] = initramfs_vnode;
        break;
      }
    }
  }
  for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
    initramfs_vnode->subdirs[i] = (vnode *)0;
  }
  initramfs_vnode->mountpoint = (mount *)0;
  initramfs_vnode->v_ops = initramfs_v_ops;
  initramfs_vnode->f_ops = initramfs_f_ops;
  initramfs_vnode->type = type;
  if (type == REGULAR_FILE) {
    uint32_t file_sz;
    char *file_addr = cpio_load(name, &file_sz);
    initramfs_internal *internal =
        (initramfs_internal *)malloc(sizeof(initramfs_internal));
    // the data in initramfs internal is pointed to the start address of the
    // cpio file content
    internal->file_sz = file_sz;
    internal->data = file_addr;
    initramfs_vnode->internal = (void *)internal;
  } else {
    initramfs_vnode->internal = (void *)0;
    void *addr = cpio_start_addr;
    while (strcmp((char *)(addr + sizeof(cpio_header)), "TRAILER!!!") != 0) {
      cpio_header *header = (cpio_header *)addr;
      uint32_t filename_size =
          cpio_atoi(header->c_namesize, (int)sizeof(header->c_namesize));
      uint32_t headerPathname_size = sizeof(cpio_header) + filename_size;
      uint32_t file_size =
          cpio_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
      align_inplace(&headerPathname_size, 4);
      align_inplace(&file_size, 4);
      initramfs_create_vnode(initramfs_vnode, addr + sizeof(cpio_header),
                             REGULAR_FILE);

      addr += (headerPathname_size + file_size);
    }
  }
  return initramfs_vnode;
}

static int32_t initramfs_vnode_create(vnode *dir_node, vnode **target,
                                      const char *component_name) {
  uart_puts("create operation fails on initramfs");
  return -1;
}

static int32_t initramfs_vnode_lookup(vnode *dir_node, vnode **target,
                                      const char *component_name) {
  for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
    if (dir_node->subdirs[i] &&
        !strcmp(dir_node->subdirs[i]->name, component_name)) {
      *target = dir_node->subdirs[i];
      return 0;
    }
  }
  return -1;
}

static int32_t initramfs_vnode_mkdir(vnode *dir_node, vnode **target,
                                     const char *component_name) {
  uart_puts("mkdir operation fails on initramfs");
  return -1;
}

static file *initramfs_create_fd(vnode *file_node) {
  file *fp = (file *)malloc(sizeof(file));
  if (!fp) {
    return (file *)0;
  }
  fp->vnode = file_node;
  fp->f_ops = file_node->f_ops;
  fp->f_pos = 0;
  return fp;
}

static int32_t initramfs_file_open(vnode *file_node, file **fp) {
  *fp = initramfs_create_fd(file_node);
  if (!(*fp)) {
    return -1;
  }
  return 0;
}

static int32_t initramfs_file_close(file *fp) {
  free((void *)fp);
  return 0;
}

static int32_t initramfs_file_read(file *fp, void *buf, uint32_t len) {
  initramfs_internal *internal = (initramfs_internal *)fp->vnode->internal;
  char *src = internal->data, *dst = (char *)buf;
  uint32_t i;
  for (i = 0; i < len && fp->f_pos < internal->file_sz; i++, fp->f_pos++) {
    dst[i] = src[fp->f_pos];
  }
  return i;
}

static int32_t initramfs_file_write(file *fp, const void *buf, uint32_t len) {
  uart_puts("write operation fails on initramfs");
  return -1;
}

static int32_t initramfs_setup_mount(filesystem *fs, mount *mount,
                                     const char *name) {
  mount->fs = fs;
  mount->root = initramfs_create_vnode((vnode *)0, name, DIRECTORY);
  if (!mount->root) {
    return -1;
  }
  mount->root->mountpoint = mount;
  return 0;
}

static int32_t initramfs_register_fs() {
  if (!initramfs_v_ops) {
    initramfs_v_ops = (vnode_operations *)malloc(sizeof(vnode_operations));
    if (!initramfs_v_ops) {
      return -1;
    }
    initramfs_v_ops->create =
        (vfs_create_fp_t)phy2vir((uint64_t)initramfs_vnode_create);
    initramfs_v_ops->lookup =
        (vfs_lookup_fp_t)phy2vir((uint64_t)initramfs_vnode_lookup);
    initramfs_v_ops->mkdir =
        (vfs_mkdir_fp_t)phy2vir((uint64_t)initramfs_vnode_mkdir);
  }

  if (!initramfs_f_ops) {
    initramfs_f_ops = (file_operations *)malloc(sizeof(file_operations));
    if (!initramfs_f_ops) {
      return -1;
    }
    initramfs_f_ops->open =
        (vfs_open_fp_t)phy2vir((uint64_t)initramfs_file_open);
    initramfs_f_ops->close =
        (vfs_close_fp_t)phy2vir((uint64_t)initramfs_file_close);
    initramfs_f_ops->read =
        (vfs_read_fp_t)phy2vir((uint64_t)initramfs_file_read);
    initramfs_f_ops->write =
        (vfs_write_fp_t)phy2vir((uint64_t)initramfs_file_write);
  }
  return 0;
}

filesystem *initramfs_init() {
  if (!initramfs) {  // if the initramfs has not been initialized
    initramfs = (filesystem *)malloc(sizeof(filesystem));
    strcpy(initramfs->name, "initramfs");
    initramfs->setup_mnt =
        (vfs_setup_mount)phy2vir((uint64_t)initramfs_setup_mount);
    initramfs->register_fs =
        (vfs_register_fs)phy2vir((uint64_t)initramfs_register_fs);
  }
  return initramfs;
}