#include "vfs_tmpfs.h"
#include "mm.h"
#include "string.h"
#include "vfs.h"

struct file_operations tmpfs_file_ops = {
    .open = tmpfs_open,
    .close = tmpfs_close,
    .read = tmpfs_read,
    .write = tmpfs_write,
    .lseek64 = tmpfs_lseek64,
};

struct vnode_operations tmpfs_vnode_ops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir,
};

int tmpfs_register()
{
    struct filesystem fs;
    fs.name = "tmpfs";
    fs.setup_mount = tmpfs_setup_mount;
    return register_filesystem(&fs);
}

struct vnode *tmpfs_create_vnode(enum fsnode_type type)
{
    struct tmpfs_vnode *tmpfs_vnode = kmalloc(sizeof(struct tmpfs_vnode));
    memset(tmpfs_vnode, 0, sizeof(struct tmpfs_vnode));
    tmpfs_vnode->type = type;
    tmpfs_vnode->data = kmalloc(TMPFS_MAX_FILE_SIZE);
    memset(tmpfs_vnode->data, 0, TMPFS_MAX_FILE_SIZE);

    struct vnode *vnode = kmalloc(sizeof(struct vnode));
    vnode->mount = 0;
    vnode->f_ops = &tmpfs_file_ops;
    vnode->v_ops = &tmpfs_vnode_ops;
    vnode->internal = tmpfs_vnode;
    return vnode;
}

int tmpfs_setup_mount(struct filesystem *fs, struct mount *mnt)
{
    mnt->root = tmpfs_create_vnode(FS_DIR);
    mnt->fs = fs;
    return 0;
}

int tmpfs_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = &tmpfs_file_ops;
    (*target)->f_pos = 0;
    return 0;
}

int tmpfs_close(struct file *file)
{
    kfree(file);
    return 0;
}

int tmpfs_read(struct file *file, void *buf, size_t len)
{
    struct tmpfs_vnode *inode = file->vnode->internal;
    if (file->f_pos + len > inode->datasize)
        len = inode->datasize - file->f_pos;
    memcpy(buf, inode->data + file->f_pos, len);
    file->f_pos += len;
    return len;
}

int tmpfs_write(struct file *file, const void *buf, size_t len)
{
    struct tmpfs_vnode *inode = file->vnode->internal;
    memcpy(inode->data + file->f_pos, buf, len);
    file->f_pos += len;
    if (inode->datasize < file->f_pos)
        inode->datasize = file->f_pos;
    return len;
}

long tmpfs_lseek64(struct file *file, long offset, int whence)
{
    if (whence == SEEK_SET) {
        file->f_pos = offset;
        return offset;
    }
    return -1;
}

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target,
                 const char *component_name)
{
    struct tmpfs_vnode *dentry = dir_node->internal;
    for (int i = 0; i < TMPFS_MAX_DIR_ENTRY; i++) {
        if (!dentry->entry[i])
            return -1;
        struct tmpfs_vnode *inode = dentry->entry[i]->internal;
        if (!strcmp(inode->name, component_name)) {
            *target = dentry->entry[i];
            return 0;
        }
    }
    return -1;
}

int tmpfs_create(struct vnode *dir_node, struct vnode **target,
                 const char *component_name)
{
    struct tmpfs_vnode *inode = dir_node->internal;

    if (inode->type != FS_DIR)
        return -1; // Not a directory

    int idx = 0;
    while (idx < TMPFS_MAX_DIR_ENTRY && inode->entry[idx]) {
        struct tmpfs_vnode *entry_inode = inode->entry[idx]->internal;
        if (!strcmp(entry_inode->name, component_name))
            return -1; // File exists
        idx++;
    }

    struct vnode *vnode = tmpfs_create_vnode(FS_FILE);
    inode->entry[idx] = vnode;

    struct tmpfs_vnode *entry_inode = vnode->internal;
    strncpy(entry_inode->name, component_name, strlen(component_name));

    *target = vnode;
    return 0;
}

int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target,
                const char *component_name)
{
    struct tmpfs_vnode *inode = dir_node->internal;
    int idx = 0;
    while (idx < TMPFS_MAX_DIR_ENTRY && inode->entry[idx])
        idx++;

    struct vnode *vnode = tmpfs_create_vnode(FS_DIR);
    inode->entry[idx] = vnode;

    struct tmpfs_vnode *entry_inode = vnode->internal;
    strncpy(entry_inode->name, component_name, strlen(component_name));

    *target = vnode;
    return 0;
}
