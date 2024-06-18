#include "tmpfs.h"
#include "uart.h"
#include "mm.h"
#include "string.h"

struct file_operations tmpfs_file_operations = {
    .write   = tmpfs_write,
    .read    = tmpfs_read,
    .open    = tmpfs_open,
    .close   = tmpfs_close,
    .lseek64 = tmpfs_lseek64,
};

struct vnode_operations tmpfs_vnode_operations = {
    .lookup  = tmpfs_lookup,
    .create  = tmpfs_create,
    .mkdir   = tmpfs_mkdir,
};

int tmpfs_setup_mount(struct filesystem* fs, struct mount* mount)
{
#ifdef VFS_DEBUG
    printf("        [tmpfs_setup_mount]\n");
#endif

    mount->fs = fs;
    mount->root = tmpfs_create_vnode(0, FSNODE_TYPE_DIR);
    return 1;
}

/**
 * Create a tmpfs vnode along with the inode below it.
 * We should specify the type of the vnode and the mount structure of it.
 */
struct vnode *tmpfs_create_vnode(struct mount *mount, enum fsnode_type type)
{
    struct vnode *node;
    struct tmpfs_inode *inode;
    int i;

    /* Create vnode */
    node = (struct vnode*)kmalloc(sizeof(struct vnode));
    node->v_ops = &tmpfs_vnode_operations;
    node->f_ops = &tmpfs_file_operations;
    node->mount = mount; // All the vnode should not point back to mount.

    /* Create tmpfs inode */
    inode = (struct tmpfs_inode *)kmalloc(sizeof(struct tmpfs_inode));
    memset(inode, 0, sizeof(struct tmpfs_inode));
    inode->type = type;
    for (i = 0; i < MAX_DIR_NUM; i++)
        inode->childs[i] = 0;
    inode->data = (char *)kmalloc(DEFAULT_INODE_SIZE);

    /* Assign inode to vnode */
    node->internal = inode;
    return node;
}

struct filesystem tmpfs_filesystem = {
    .name = "tmpfs",
    .setup_mount = tmpfs_setup_mount,
};

/* file write operation */
int tmpfs_write(struct file *file, const void *buf, size_t len)
{
    struct tmpfs_inode *inode;

    if (!len)
        return 0;
    /* Take the file inode. */
    inode = file->vnode->internal;
    /* Copy data from buf to f_pos */
    memcpy(inode->data + file->f_pos, buf, len);
    /* Update f_pos and size */
    file->f_pos += len;

    /* Check boundary. */
    if (inode->data_size < file->f_pos)
        inode->data_size = file->f_pos;
    return len;
}

/* File read operation */
int tmpfs_read(struct file *file, void *buf, size_t len)
{
    struct tmpfs_inode *inode;

    inode = file->vnode->internal;
    /*if buffer overflow, shrink the request read length. Then read from f_pos */
    if ((file->f_pos + len) > inode->data_size) {
        len = inode->data_size - file->f_pos;
        memcpy(buf, inode->data + file->f_pos, len);
        file->f_pos += inode->data_size - file->f_pos;
    } else {
        memcpy(buf, inode->data + file->f_pos, len);
        file->f_pos += len;
    }
    return len;
}

/* Newly created struct file should be generated by file_vnode. */
int tmpfs_open(struct vnode *file_node, struct file **target)
{
    (*target)->vnode = file_node;
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    return 1;
}

/* file close opeation. Just free the memory */
int tmpfs_close(struct file *file)
{
    kfree(file);
    return 1;
}

/* Move the f_pos of the struct file. */
long tmpfs_lseek64(struct file *file, long offset, int whence)
{
    if (whence == SEEK_SET) {
        file->f_pos = offset;
        return file->f_pos;
    }
    return 0;
}

/**
 * tmpfs vnode operations, lookup all the vnodes under dir_node.
 * Returns 0 if error, returns 1 if success.
 */
int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *dir_inode, *inode;
    struct vnode *vnode;
    int child_idx;

    /* Search the child inode. */
    dir_inode = dir_node->internal;
    for (child_idx = 0; child_idx < MAX_DIR_NUM; child_idx++) {
        vnode = dir_inode->childs[child_idx];
        if (!vnode)
            break;
        inode = vnode->internal;
        if (!strcmp(component_name, inode->name)) {
            *target = vnode;
            return 1;
        }
    }
#ifdef VFS_DEBUG
    printf("        [tmpfs_lookup] %s not found\n", component_name);
#endif
    return 0;
}

/* tmpfs vnode operation: create a vnode under dir_node, put it to target */
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *inode, *child_inode, *new_inode;
    int child_idx;
    struct vnode *new_vnode;

#ifdef VFS_DEBUG
    printf("        [tmpfs_create] component name: %s\n", component_name);
#endif

    /* Get the inode of dir_node*/
    inode = dir_node->internal;
    if (inode->type != FSNODE_TYPE_DIR) {
        printf("        [tmpfs_create] not under directory vnode\n");
        return 0;
    }

    for (child_idx = 0; child_idx < MAX_DIR_NUM; child_idx++) {
        if (!inode->childs[child_idx])
            break;
        /* Get the child inode and compare the name. */
        child_inode = inode->childs[child_idx]->internal;
        if (!strcmp(child_inode->name, component_name)) {
            printf("        [tmpfs_create] file %s already exists\n", component_name);
            return 0;
        }
    }

    if (child_idx >= MAX_DIR_NUM) {
        printf("        [tmpfs_create] directory entry full.\n");
        return 0;
    }

    if (strlen(component_name) > MAX_PATH_LEN) {
        printf("        [tmpfs_create] file name too long.\n");
        return 0;
    }

    /* Create a new vnode and assign it to founded index. */
    new_vnode = tmpfs_create_vnode(0, FSNODE_TYPE_FILE);
    inode->childs[child_idx] = new_vnode;

    /* Copy the component_name to inode. */
    new_inode = new_vnode->internal;
    strcpy(new_inode->name, component_name);

    *target = new_vnode;
    return 1;
}

/* vnode operation: create a directory vnode under dir_node, put it to target */
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    struct tmpfs_inode *inode, *new_inode;
    int child_idx;
    struct vnode* new_vnode;

    inode = dir_node->internal;
    if (inode->type != FSNODE_TYPE_DIR) {
        printf("        [tmpfs_mkdir] dir_node is not directory\n");
        return -1;
    }

    /* Find the available child_idx. */
    for (child_idx = 0; child_idx < MAX_DIR_NUM; child_idx++)
        if (!inode->childs[child_idx])
            break;
    if (child_idx >= MAX_DIR_NUM) {
        printf("        [tmpfs_mkdir] directory entry full\n");
        return -1;
    }

    if (strlen(component_name) > MAX_PATH_LEN) {
        printf("        [tmpfs_mkdir] file name too long\n");
        return -1;
    }

    /* Create a new vnode and put it to inode->childs. */
    new_vnode = tmpfs_create_vnode(0, FSNODE_TYPE_DIR);
    inode->childs[child_idx] = new_vnode;

    /* Assign component name to the inode->name. */
    new_inode = new_vnode->internal;
    strcpy(new_inode->name, component_name);

    *target = new_vnode;
    return 0;
}