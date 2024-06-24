#include "tmpfs.h"
#include "memory.h"
#include "string.h"
#include "uart1.h"
#include "vfs.h"

struct file_operations tmpfs_file_ops = {
    .write = tmpfs_write,
    .read = tmpfs_read,
    .open = tmpfs_open,
    .close = tmpfs_close,
    .lseek64 = tmpfs_lseek64,
    .getsize = tmpfs_getsize,
};
struct vnode_operations tmpfs_vnode_ops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir,
};

int register_tmpfs()
{
    struct filesystem fs;
    fs.name = "tmpfs";
    fs.setup_mount = tmpfs_setup_mount;
    return register_filesystem(&fs);
}

int tmpfs_setup_mount(struct filesystem *fs, struct mount *_mount)
{
    _mount->fs = fs;
    _mount->root = tmpfs_create_vnode(_mount, TMPFS_DIR);
    return 0;
}

struct vnode *tmpfs_create_vnode(struct mount *_mount, enum fsnode_type type)
{
    struct vnode *v = kmalloc(sizeof(struct vnode));
    v->f_ops = &tmpfs_file_ops;
    v->v_ops = &tmpfs_vnode_ops;
    v->mount = 0;

    struct tmpfs_inode *inode = kmalloc(sizeof(struct tmpfs_inode));
    memset(inode, 0, sizeof(struct tmpfs_inode));
    inode->type = type;
    inode->data = kmalloc(0x1000);

    v->internal = inode;
    return v;
}

int tmpfs_write(struct file *file, const void *buf, size_t len)
{
    struct tmpfs_inode *inode = file->vnode->internal;

    memcpy(inode->data + file->f_pos, buf, len);
    file->f_pos += len;

    if (file->f_pos > inode->datasize) {
        inode->datasize = file->f_pos;
    }

    return len;
}

int tmpfs_read(struct file *file, void *buf, size_t len)
{
    struct tmpfs_inode *inode = file->vnode->internal;

    // when len + file->f_pos > inode->datasize, return the remaining data
    if (len + file->f_pos > inode->datasize)
        len = inode->datasize - file->f_pos;

    memcpy(buf, inode->data + file->f_pos, len);
    file->f_pos += len;
    return len;
}

int tmpfs_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    return 0;
}

int tmpfs_close(struct file *file)
{
    kfree(file);
    return 0;
}

long tmpfs_lseek64(struct file *file, long offset, int whence)
{
    if (whence == SEEK_SET) {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1; // if whence is not SEEK_SET
}

long tmpfs_getsize(struct vnode *vd)
{
    struct tmpfs_inode *inode = vd->internal;
    return inode->datasize;
}

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *dir_inode = dir_node->internal;
    int child_idx = 0;
    for (; child_idx < MAX_DIR_ENTRY; child_idx++) {
        struct vnode *vnode = dir_inode->entry[child_idx];
        if (!vnode)
            break;

        struct tmpfs_inode *inode = vnode->internal;
        if (strcmp(component_name, inode->name) == 0) {
            *target = vnode;
            return 0;
        }
    }

    return -1;
}

int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *inode = dir_node->internal;
    if (inode->type != TMPFS_DIR)
        return -1; // not a directory

    int child_idx = 0;
    for (; child_idx < MAX_DIR_ENTRY; child_idx++) {
        if (!inode->entry[child_idx])
            break; // found an empty slot

        struct tmpfs_inode *child_inode = inode->entry[child_idx]->internal;
        if (strcmp(child_inode->name, component_name) == 0) {
            // uart_printf("tmpfs create file exists\n");
            return -1; // file exists
        }
    }

    if (child_idx == MAX_DIR_ENTRY)
        return -1; // directory full

    struct vnode *_vnode = tmpfs_create_vnode(0, TMPFS_FILE);
    inode->entry[child_idx] = _vnode;

    if (strlen(component_name) > FILE_NAME_MAX)
        return -1; // name too long

    struct tmpfs_inode *new_inode = _vnode->internal;
    strcpy(new_inode->name, component_name);

    *target = _vnode;
    return 0;
}

int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *inode = dir_node->internal;
    if (inode->type != TMPFS_DIR)
        return -1; // not a directory

    int child_idx = 0;
    for (; child_idx < MAX_DIR_ENTRY; child_idx++) {
        if (!inode->entry[child_idx])
            break; // found an empty slot
    }

    if (child_idx == MAX_DIR_ENTRY)
        return -1; // directory full

    if (strlen(component_name) > FILE_NAME_MAX)
        return -1; // name too long

    struct vnode *_vnode = tmpfs_create_vnode(dir_node->mount, TMPFS_DIR);
    inode->entry[child_idx] = _vnode;

    struct tmpfs_inode *new_inode = _vnode->internal;
    strcpy(new_inode->name, component_name);

    *target = _vnode;
    return 0;
}
