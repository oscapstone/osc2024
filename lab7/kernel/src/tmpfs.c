#include "allocator.h"
#include "utils.h"
#include "tmpfs.h"

static struct file_operations tmpfs_f_ops = {
    .write = tmpfs_write,
    .read = tmpfs_read};

static struct inode_operations tmpfs_i_ops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create};

int tmpfs_mount(struct filesystem *fs, struct mount *mount)
{
    mount->root = kmalloc(sizeof(struct dentry));
    my_strcpy(mount->root->d_name, "/");
    mount->root->d_parent = NULL;

    mount->root->d_inode = kmalloc(sizeof(struct inode));
    mount->root->d_inode->f_ops = &tmpfs_f_ops;
    mount->root->d_inode->i_ops = &tmpfs_i_ops;
    mount->root->d_inode->i_dentry = mount->root;
    mount->root->d_inode->internal = NULL;

    for (int i = 0; i < 16; i++)
        mount->root->d_subdirs[i] = NULL;

    mount->fs = fs;

    return 1;
}

int tmpfs_write(struct file *file, const void *buf, size_t len)
{
    struct tmpfs_internal *tmpfs_internal = (struct tmpfs_internal *)file->f_dentry->d_inode->internal;

    char *dest = &tmpfs_internal->data[file->f_pos];
    char *src = (char *)buf;

    int i = 0;
    for (; i < len && src[i] != '\0'; i++)
        dest[i] = src[i];
    dest[i] = EOF;

    return i;
}

int tmpfs_read(struct file *file, void *buf, size_t len)
{
    struct tmpfs_internal *tmpfs_internal = (struct tmpfs_internal *)file->f_dentry->d_inode->internal;

    char *dest = (char *)buf;
    char *src = &(tmpfs_internal->data[file->f_pos]);
    int i = 0;
    for (; i < len && src[i] != (unsigned char)EOF; i++)
        dest[i] = src[i];

    return i;
}

struct dentry *tmpfs_lookup(struct inode *dir, const char *component_name)
{
    struct dentry *cur = dir->i_dentry;
    for (int i = 0; i < 16; i++)
    {
        if (cur->d_subdirs[i] != NULL && my_strcmp(cur->d_subdirs[i]->d_name, component_name) == 0)
            return cur->d_subdirs[i];
    }

    return NULL;
}

int tmpfs_create(struct inode *dir, struct dentry *dentry, int mode)
{
    struct dentry *cur = dir->i_dentry;
    for (int i = 0; i < 16; i++)
    {
        if (cur->d_subdirs[i] == NULL)
        {
            cur->d_subdirs[i] = dentry;
            break;
        }
    }

    dentry->d_inode = kmalloc(sizeof(struct inode));
    dentry->d_inode->f_ops = &tmpfs_f_ops;
    dentry->d_inode->i_ops = &tmpfs_i_ops;
    dentry->d_inode->i_dentry = dentry;
    dentry->d_inode->internal = kmalloc(sizeof(struct tmpfs_internal));
    ((struct tmpfs_internal *)(dentry->d_inode->internal))->data[0] = EOF;

    return 1;
}