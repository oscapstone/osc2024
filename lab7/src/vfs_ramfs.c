#include "vfs_ramfs.h"
#include "initrd.h"
#include "mm.h"
#include "string.h"
#include "uart.h"
#include "utils.h"

extern void *RAMFS_BASE;

struct file_operations ramfs_file_ops = {
    .open = ramfs_open,
    .close = ramfs_close,
    .read = ramfs_read,
    .write = ramfs_write,
    .lseek64 = ramfs_lseek64,
};

struct vnode_operations ramfs_vnode_ops = {
    .lookup = ramfs_lookup,
    .create = ramfs_create,
    .mkdir = ramfs_mkdir,
};

int ramfs_register()
{
    struct filesystem fs;
    fs.name = "initramfs";
    fs.setup_mount = ramfs_setup_mount;
    return register_filesystem(&fs);
}

struct vnode *ramfs_create_vnode(struct mount *mnt, enum fsnode_type type)
{
    struct ramfs_vnode *ramfs_vnode = kmalloc(sizeof(struct ramfs_vnode));
    memset(ramfs_vnode, 0, sizeof(struct ramfs_vnode));
    ramfs_vnode->type = type;
    ramfs_vnode->data = kmalloc(RAMFS_MAX_FILE_SIZE);
    memset(ramfs_vnode->data, 0, RAMFS_MAX_FILE_SIZE);

    struct vnode *vnode = kmalloc(sizeof(struct vnode));
    vnode->mount = mnt;
    vnode->f_ops = &ramfs_file_ops;
    vnode->v_ops = &ramfs_vnode_ops;
    vnode->internal = ramfs_vnode;

    return vnode;
}

int ramfs_setup_mount(struct filesystem *fs, struct mount *mnt)
{
    mnt->root = ramfs_create_vnode(0, FS_DIR);
    mnt->fs = fs;

    struct ramfs_vnode *ramfs_root = mnt->root->internal;

    char *fptr = (char *)RAMFS_BASE;
    int idx = 0;
    while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
        cpio_t *header = (cpio_t *)fptr;
        int namesize = hextoi(header->c_namesize, 8);
        int filesize = hextoi(header->c_filesize, 8);
        int headsize = align4(sizeof(cpio_t) + namesize);
        int datasize = align4(filesize);

        struct vnode *vnode = ramfs_create_vnode(0, FS_FILE);
        struct ramfs_vnode *inode = vnode->internal;
        inode->name = kmalloc(namesize);
        strncpy(inode->name, fptr + sizeof(cpio_t), namesize);
        inode->data = kmalloc(filesize);
        memcpy(inode->data, fptr + headsize, filesize);
        inode->datasize = filesize;
        ramfs_root->entry[idx++] = vnode;

        fptr += headsize + datasize;
    }

    return 0;
}

int ramfs_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    return 0;
}

int ramfs_close(struct file *file)
{
    // TODO: kfree(file);
    return 0;
}

int ramfs_read(struct file *file, void *buf, size_t len)
{
    struct ramfs_vnode *inode = file->vnode->internal;
    if (file->f_pos + len > inode->datasize)
        len = inode->datasize - file->f_pos;
    memcpy(buf, inode->data + file->f_pos, len);
    file->f_pos += len;
    return len;
}

int ramfs_write(struct file *file, const void *buf, size_t len)
{
    return -1;
}

long ramfs_lseek64(struct file *file, long offset, int whence)
{
    return -1;
}

int ramfs_lookup(struct vnode *dir_node, struct vnode **target,
                 const char *component_name)
{
    struct ramfs_vnode *dentry = dir_node->internal;
    for (int i = 0; i < RAMFS_MAX_DIR_ENTRY; i++) {
        if (!dentry->entry[i])
            return -1;
        struct ramfs_vnode *inode = dentry->entry[i]->internal;
        if (!strcmp(inode->name, component_name)) {
            *target = dentry->entry[i];
            return 0;
        }
    }
    return -1;
}

int ramfs_create(struct vnode *dir_node, struct vnode **target,
                 const char *component_name)
{
    return -1;
}

int ramfs_mkdir(struct vnode *dir_node, struct vnode **target,
                const char *component_name)
{
    return -1;
}
