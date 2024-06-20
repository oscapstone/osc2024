#include "vfs_tmpfs.h"
#include "vfs.h"
#include "string.h"
#include "memory.h"
#include "uart1.h"

struct file_operations tmpfs_file_operations = {
    tmpfs_write, tmpfs_read, tmpfs_open,
    tmpfs_close, tmpfs_lseek64, tmpfs_getsize};

struct vnode_operations tmpfs_vnode_operations = {
    tmpfs_lookup, tmpfs_create, tmpfs_mkdir,
    (void *)tmpfs_op_deny,(void *)tmpfs_op_deny,(void *)tmpfs_op_deny,
    tmpfs_ls, tmpfs_dump};

int register_tmpfs()
{
    struct filesystem fs;
    fs.name = "tmpfs";
    fs.setup_mount = tmpfs_setup_mount;
    fs.sync = tmpfs_sync;
    return register_filesystem(&fs);
}

int tmpfs_setup_mount(struct filesystem *fs, struct mount *_mount)
{
    _mount->fs = fs;
    _mount->root = tmpfs_create_vnode(0, dir_t);
    return 0;
}

struct vnode *tmpfs_create_vnode(struct mount *_mount, enum fsnode_type type)
{
    struct vnode *v = kmalloc(sizeof(struct vnode));
    v->f_ops = &tmpfs_file_operations;
    v->v_ops = &tmpfs_vnode_operations;
    v->mount = 0;
    struct tmpfs_inode *inode = kmalloc(sizeof(struct tmpfs_inode));
    memset(inode, 0, sizeof(struct tmpfs_inode));
    inode->type = type;
    inode->data = kmalloc(0x1000);
    v->internal = inode;
    return v;
}

// vnode operations
int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *dir_inode = dir_node->internal;
    int child_idx = 0;
    // BFS search tree
    for (; child_idx <= MAX_DIR_ENTRY; child_idx++)
    {
        struct vnode *vnode = dir_inode->entry[child_idx];
        if (!vnode)
            break;
        struct tmpfs_inode *inode = vnode->internal;
        if (strcmp(component_name, inode->name) == 0)
        {
            *target = vnode;
            return 0;
        }
    }
    return -1;
}

// dir ops
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *inode = dir_node->internal;

    if (inode->type != dir_t)
    {
        uart_sendlinek("tmpfs mkdir not dir_t\r\n");
        return -1;
    }

    int child_idx = 0;
    for (; child_idx <= MAX_DIR_ENTRY; child_idx++)
    {
        if (!inode->entry[child_idx])
        {
            break;
        }
    }

    if (child_idx > MAX_DIR_ENTRY)
    {
        uart_sendlinek("DIR ENTRY FULL\r\n");
        return -1;
    }

    if (strlen(component_name) > MAX_FILE_NAME)
    {
        uart_sendlinek("FILE NAME TOO LONG\r\n");
        return -1;
    }

    struct vnode *_vnode = tmpfs_create_vnode(0, dir_t);
    inode->entry[child_idx] = _vnode;

    struct tmpfs_inode *newinode = _vnode->internal;
    strcpy(newinode->name, component_name);

    *target = _vnode;
    return 0;
}

// file operations
int tmpfs_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    return 0;
}

// file operations
int tmpfs_write(struct file *file, const void *buf, size_t len)
{
    struct tmpfs_inode *inode = file->vnode->internal;
    // write from f_pos
    memcpy(inode->data + file->f_pos, buf, len);
    // update f_pos and size
    file->f_pos += len;
    if (inode->datasize < file->f_pos)
        inode->datasize = file->f_pos;
    return len;
}

int tmpfs_read(struct file *file, void *buf, size_t len)
{
    struct tmpfs_inode *inode = file->vnode->internal;
    // if buffer overflow, shrink the request read length
    // read from f_pos
    if (len + file->f_pos > inode->datasize)
    {
        len = inode->datasize - file->f_pos;
        memcpy(buf, inode->data + file->f_pos, len);
        file->f_pos += inode->datasize - file->f_pos;
        return len;
    }
    else
    {
        memcpy(buf, inode->data + file->f_pos, len);
        file->f_pos += len;
        return len;
    }
    return -1;
}

int tmpfs_close(struct file *file)
{
    kfree(file);
    return 0;
}

long tmpfs_lseek64(struct file *file, long offset, int whence)
{
    if (whence == SEEK_SET)
    {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}

// file ops
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *inode = dir_node->internal;
    if (inode->type != dir_t)
    {
        uart_sendlinek("tmpfs create not dir_t\r\n");
        return -1;
    }

    int child_idx = 0;
    for (; child_idx <= MAX_DIR_ENTRY; child_idx++)
    {
        if (!inode->entry[child_idx])
            break;
        struct tmpfs_inode *child_inode = inode->entry[child_idx]->internal;
        if (strcmp(child_inode->name, component_name) == 0)
        {
            uart_sendlinek("tmpfs create file exists\r\n");
            return -1;
        }
    }

    if (child_idx > MAX_DIR_ENTRY)
    {
        uart_sendlinek("DIR ENTRY FULL\r\n");
        return -1;
    }

    if (strlen(component_name) > MAX_FILE_NAME)
    {
        uart_sendlinek("FILE NAME TOO LONG\r\n");
        return -1;
    }

    struct vnode *_vnode = tmpfs_create_vnode(0, file_t);
    inode->entry[child_idx] = _vnode;

    struct tmpfs_inode *newinode = _vnode->internal;
    strcpy(newinode->name, component_name);

    *target = _vnode;
    return 0;
}

long tmpfs_getsize(struct vnode *vd)
{
    struct tmpfs_inode *inode = vd->internal;
    return inode->datasize;
}

void tmpfs_dump(struct vnode *vnode, int level)
{
    struct tmpfs_inode *inode = (struct tmpfs_inode *)vnode->internal;

    uart_sendlinek("+-%s", inode->name);
    switch (inode->type)
    {
    case dir_t:
        uart_sendlinek("(dir)\n");
        break;
    case file_t:
        uart_sendlinek("(file)\n");
        break;

    default:
        break;
    }
    int child_idx = 0;
    for (; child_idx <= MAX_DIR_ENTRY; child_idx++)
    {
        if (!inode->entry[child_idx])
        {
            break;
        }
        vfs_dump(inode->entry[child_idx], level + 1);
    }
}

void tmpfs_ls(struct vnode *vnode)
{
    struct tmpfs_inode *inode = (struct tmpfs_inode *)vnode->internal;
    int child_idx = 0;
    for (; child_idx <= MAX_DIR_ENTRY; child_idx++)
    {
        if (!inode->entry[child_idx])
        {
            break;
        }
        vnode = inode->entry[child_idx];
        uart_sendlinek("%s\n", ((struct tmpfs_inode *)vnode->internal)->name);
    }
}

int tmpfs_op_deny()
{
    return -1;
}

int tmpfs_sync(struct filesystem *fs)
{
    return 0;
}
