#include "initramfs.h"
#include "vfs.h"
#include "utils.h"
#include "memory.h"
#include "uart1.h"
#include "cpio.h"

struct file_operations initramfs_file_operations = {initramfs_write, initramfs_read, initramfs_open, initramfs_close, initramfs_lseek64, initramfs_getsize};
struct vnode_operations initramfs_vnode_operations = {initramfs_lookup, initramfs_create, initramfs_mkdir};

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
    _mount->root = initramfs_create_vnode(dir_t);
    // create entry under _mount, cpio files should be attached on it
    struct initramfs_inode *ramdir_inode = _mount->root->internal;

    // add all file in initramfs to filesystem
    char *filepath;
    char *filedata;
    unsigned int filesize;
    struct cpio_newc_header *header_pointer = CPIO_DEFAULT_START;
    int idx = 0;

    while (header_pointer != 0)
    {
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        // if parse header error
        if (error)
        {
            uart_sendline("%s", "error\r\n");
            break;
        }

        // if this is not TRAILER!!! (last of file)
        if (header_pointer != 0)
        {
            // only support file (no dir)
            struct vnode *filevnode = initramfs_create_vnode(file_t);
            struct initramfs_inode *fileinode = filevnode->internal;
            fileinode->name = filepath;
            fileinode->data = filedata;
            fileinode->datasize = filesize;
            ramdir_inode->entry[idx++] = filevnode;
        }
    }

    return 0;
}

struct vnode *initramfs_create_vnode(enum fsnode_type type)
{
    struct vnode *v = kmalloc(sizeof(struct vnode));
    v->mount = 0;
    v->f_ops = &initramfs_file_operations;
    v->v_ops = &initramfs_vnode_operations;
    struct initramfs_inode *inode = kmalloc(sizeof(struct initramfs_inode));
    memset(inode, 0, sizeof(struct initramfs_inode));
    inode->type = type;
    inode->data = kmalloc(0x1000);
    v->internal = inode;
    return v;
}

// file operations
int initramfs_write(struct file *file, const void *buf, size_t len)
{
    // read-only
    return -1;
}

int initramfs_read(struct file *file, void *buf, size_t len)
{
    struct initramfs_inode *inode = file->vnode->internal;
    // overflow, shrink size
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
    if (whence == SEEK_SET)
    {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}

// vnode operations
int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct initramfs_inode *dir_inode = dir_node->internal;
    for (int child_idx = 0; child_idx < INITRAMFS_MAX_DIR_ENTRY; child_idx++)
    {
        struct vnode *vnode = dir_inode->entry[child_idx];
        if (!vnode)
            break;
        struct initramfs_inode *inode = vnode->internal;
        if (strcmp(component_name, inode->name) == 0)
        {
            *target = vnode;
            return 0;
        }
    }
    return -1;
}

int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    // read-only
    return -1;
}

int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    // read-only
    return -1;
}

long initramfs_getsize(struct vnode *vd)
{
    struct initramfs_inode *inode = vd->internal;
    return inode->datasize;
}
