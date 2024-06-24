#ifndef _FS_CPIO_H
#define _FS_CPIO_H

#include "fs_vfs.h"
#include "initrd.h"
#include "list.h"
#include "string.h"

#define TMPFS_FILE_MAXSIZE PAGE_SIZE // define in the spec
#define TMPFS_DIR_MAXSIZE 16
#define TMPFS_NAME_MAXLEN 16

#define CPIOFS_TYPE_UNDEFINE 0x0
#define CPIOFS_TYPE_FILE     0x1
#define CPIOFS_TYPE_DIR      0x2

#define CPIO_TYPE_MASK  0060000
#define CPIO_TYPE_DIR   0040000
#define CPIO_TYPE_FILE  0000000

// init fs
filesystem *cpiofs_init(void);

// static methods
int cpiofs_mount(filesystem *fs, mount *mnt);

extern filesystem static_cpiofs;

int cpiofs_lookup(vnode *dir_node, vnode **target, const char *component_name);
int cpiofs_create(vnode *dir_node, vnode **target, const char *component_name);
int cpiofs_mkdir(vnode *dir_node, vnode **target, const char *component_name);
int cpiofs_isdir(vnode *dir_node);
int cpiofs_getname(vnode *dir_node, const char **name);
int cpiofs_getsize(vnode *dir_node);

extern struct vnode_operations cpiofs_v_ops;

int cpiofs_open(vnode *file_node, file *target);
int cpiofs_close(file *target);
int cpiofs_write(file *target, const void *buf, size_t len);
int cpiofs_read(file *target, void *buf, size_t len);
long cpiofs_lseek64(file *target, long offset, int whence);
int cpiofs_ioctl(struct file *file, uint64_t request, va_list args);


uint32_t cpio_read_8hex(const char *s);
vnode *get_vnode_from_path(vnode *dir_node, const char **name);
void cpio_init_mkdir(const char *pathname);

extern struct file_operations cpiofs_f_ops;

typedef struct cpiofs_file {
    const char *data;
    int size;
} cpiofs_file;

typedef struct cpiofs_dir {
    struct list_head list;
} cpiofs_dir;

typedef struct cpiofs_internal {
    const char* name;
    int type;
    union {
        cpiofs_file file;
        cpiofs_dir dir;
    };
    vnode* node;
    struct list_head list;
} cpiofs_internal;

extern vnode cpio_root_node;
extern vnode mount_old_node;
extern int cpio_mounted;

#endif // _FS_CPIO_H