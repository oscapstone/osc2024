#include "initramfs.h"
#include "cpio.h"
#include "memory.h"
#include "string.h"
#include "uart1.h"
#include "vfs.h"

struct file_operations initramfs_file_ops = {
    .write = initramfs_write,
    .read = initramfs_read,
    .open = initramfs_open,
    .close = initramfs_close,
    .lseek64 = initramfs_lseek64,
    .getsize = initramfs_getsize,
};
struct vnode_operations initramfs_vnode_ops = {
    .lookup = initramfs_lookup,
    .create = initramfs_create,
    .mkdir = initramfs_mkdir,
};

int register_initramfs()
{
    struct filesystem fs;
    fs.name = "initramfs";
    fs.setup_mount = initramfs_setup_mount;
    return register_filesystem(&fs);
}

int initramfs_setup_mount(struct filesystem *fs, struct mount *_mount)
{
    _mount->fs = fs;
    _mount->root = initramfs_create_vnode(_mount, TMPFS_DIR);

    struct initramfs_inode *ramdir_inode = _mount->root->internal; // root inode

    // add all files in initramfs to the root directory
    int idx = 0;
    unsigned int filesize;
    char *filepath, *filedata;
    struct cpio_newc_header *header = CPIO_DEFAULT_START;

    while (header != 0) {
        int error = cpio_newc_parse_header(header, &filepath, &filesize, &filedata, &header);
        if (error) {
            uart_printf("Error parsing cpio header\n");
            break;
        }

        if (header) {
            struct vnode *f_vnode = initramfs_create_vnode(0, TMPFS_FILE);
            struct initramfs_inode *f_inode = f_vnode->internal;

            f_inode->name = filepath;
            f_inode->data = filedata;
            f_inode->datasize = filesize;
            ramdir_inode->entry[idx++] = f_vnode; // add file to root directory
        }
    }

    return 0;
}

struct vnode *initramfs_create_vnode(struct mount *_mount, enum fsnode_type type)
{
    struct vnode *v = kmalloc(sizeof(struct vnode));
    v->f_ops = &initramfs_file_ops;
    v->v_ops = &initramfs_vnode_ops;
    v->mount = _mount;

    struct initramfs_inode *inode = kmalloc(sizeof(struct initramfs_inode));
    memset(inode, 0, sizeof(struct initramfs_inode));
    inode->type = type;
    inode->data = kmalloc(0x1000);

    v->internal = inode;
    return v;
}

int initramfs_write(struct file *file, const void *buf, size_t len) { return -1; }

int initramfs_read(struct file *file, void *buf, size_t len)
{
    struct initramfs_inode *inode = file->vnode->internal;
    if (len + file->f_pos > inode->datasize)
        len = inode->datasize - file->f_pos;

    memcpy(buf, inode->data + file->f_pos, len);
    file->f_pos += len;
    return len;
}

int initramfs_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    return 0;
}

int initramfs_close(struct file *file)
{
    kfree(file);
    return 0;
}

long initramfs_lseek64(struct file *file, long offset, int whence)
{
    if (whence == SEEK_SET) {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}

long initramfs_getsize(struct vnode *vd)
{
    struct initramfs_inode *inode = vd->internal;
    return inode->datasize;
}

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct initramfs_inode *dir_inode = dir_node->internal;
    int child_idx = 0;
    for (; child_idx < INITRAMFS_MAX_DIR_ENTRIES; child_idx++) {
        struct vnode *vnode = dir_inode->entry[child_idx];
        if (!vnode)
            break;

        struct initramfs_inode *inode = vnode->internal;
        if (strcmp(inode->name, component_name) == 0) {
            *target = vnode;
            return 0;
        }
    }

    return -1;
}

int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name) { return -1; }

int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name) { return -1; }
