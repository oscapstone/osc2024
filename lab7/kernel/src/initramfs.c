#include "initramfs.h"
#include "vfs.h"
#include "string.h"
#include "memory.h"
#include "cpio.h"
#include "uart1.h"
#include "colourful.h"


struct file_operations initramfs_file_operations = {
    initramfs_write, 
    initramfs_read, 
    initramfs_open, 
    initramfs_close, 
    initramfs_lseek64,
    initramfs_getsize
};
struct vnode_operations initramfs_vnode_operations = {
    initramfs_lookup, 
    initramfs_create, 
    initramfs_mkdir,
    initramfs_list,
    initramfs_gettype
};

int register_initramfs()
{
    struct filesystem fs;
    char temp[] = "initramfs";
    char *name = kmalloc(20);
    strcpy(name, temp);

    fs.name = name;
    fs.setup_mount = initramfs_setup_mount;
    return register_filesystem(&fs);
}

int initramfs_insert_the_new_entry(struct vnode *dirvnode, struct vnode *newvnode)
{
    struct initramfs_inode *dirinode = dirvnode->internal;
    int child_idx = 0;
    for (; child_idx <= INITRAMFS_MAX_DIR_ENTRY; child_idx++)
    {
        if (!dirinode->entry[child_idx]) break;
    }
    if (child_idx == INITRAMFS_MAX_DIR_ENTRY) return -1;
    dirinode->entry[child_idx] = newvnode;
    return 0;
}

int initramfs_setup_mount(struct filesystem *fs, struct mount *_mount)
{
    _mount->fs = fs;
    _mount->root = initramfs_create_vnode(0, dir_t);
    
    // create entry under _mount, cpio files should be attached on it
    // struct initramfs_inode *ramdir_inode = _mount->root->internal;

    // add all file in initramfs to filesystem
    int num_cpio = 10;
    int idx_cpio = 0;
    char *filepath[num_cpio];
    char *filedata[num_cpio];
    unsigned int filesize[num_cpio];
    struct cpio_newc_header *header_pointer = CPIO_DEFAULT_START;


    while (header_pointer != 0)
    {
        int error = cpio_newc_parse_header(header_pointer, &filepath[idx_cpio], &filesize[idx_cpio], &filedata[idx_cpio], &header_pointer);
        //if parse header error
        if (error)
        {
            uart_sendline("%s", "error\r\n");
            break;
        }

        //if this is not TRAILER!!! (last of file)
        if (header_pointer != 0)
        {
            if (strcmp(filepath[idx_cpio], ".") == 0)
            {
                continue;
            }
            idx_cpio++;

        }
    }
    idx_cpio--;

    for (; idx_cpio >= 0; idx_cpio--){
        char *sub_path[10];
        int file_num = str_SepbySomething(filepath[idx_cpio], sub_path, '/');
        int i = 0;
        struct vnode *curr_dirvnode = _mount->root;
        struct vnode *dirvnode;

        for (; i < file_num - 1; i++)
        {
            if (initramfs_lookup(curr_dirvnode, &dirvnode, sub_path[i]) != 0)
            {
                struct vnode *newdirvnode = initramfs_create_vnode(_mount, dir_t);
                struct initramfs_inode *newdirinode = newdirvnode->internal;
                newdirinode->name = sub_path[i];

                initramfs_insert_the_new_entry (curr_dirvnode, newdirvnode);

                curr_dirvnode = newdirvnode;
            }
            else
            {
                curr_dirvnode = dirvnode;
            }
        }

        if (initramfs_lookup(curr_dirvnode, &dirvnode, sub_path[i]) == 0)
        {
            continue;
        }

        struct vnode *newfilevnode;

        if (!strcmp(sub_path[i] + strlen(sub_path[i]) - 3, "img")) 
            newfilevnode = initramfs_create_vnode(_mount, img_t);
        else
            newfilevnode = initramfs_create_vnode(_mount, file_t);
        
        struct initramfs_inode *newfileinode = newfilevnode->internal;
        newfileinode->data     = filedata[idx_cpio];
        newfileinode->datasize = filesize[idx_cpio];
        newfileinode->name     = sub_path[i];
        

        initramfs_insert_the_new_entry(curr_dirvnode, newfilevnode);

    }

    return 0;
}

struct vnode *initramfs_create_vnode(struct mount *_mount, enum fsnode_type type)
{
    struct vnode *v = kmalloc(sizeof(struct vnode));
    memset(v, 0, sizeof(struct vnode));
    v->f_ops = &initramfs_file_operations;
    v->v_ops = &initramfs_vnode_operations;
    // v->mount = _mount;
    v->mount = 0;
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
        memcpy(buf, inode->data + file->f_pos, inode->datasize - file->f_pos);
        file->f_pos += inode->datasize - file->f_pos;
        return inode->datasize - file->f_pos;
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
    int child_idx = 0;
    for (; child_idx < INITRAMFS_MAX_DIR_ENTRY; child_idx++)
    {
        struct vnode *vnode = dir_inode->entry[child_idx];
        if (!vnode)break;
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

int initramfs_list(struct vnode *dir_node)
{
    struct initramfs_inode *inode = dir_node->internal;
    int child_idx = 0;
    for (; child_idx <= INITRAMFS_MAX_DIR_ENTRY; child_idx++)
    {
        if (!inode->entry[child_idx]) break;
        struct initramfs_inode *child_inode = inode->entry[child_idx]->internal;
        // uart_sendline("child_inode->type: %d\n", child_inode->type);
        if (child_inode->type == dir_t)
        {
            uart_puts( GRN "%s\t" RESET, child_inode->name );
        }
        else if (child_inode->type == img_t)
        {
            uart_puts( RED "%s\t" RESET, child_inode->name );
        }
        else if (child_inode->type == file_t)
        {
            uart_puts( "%s\t", child_inode->name);
        }
    }
    uart_sendline("\n");
    return 0;
}

enum fsnode_type initramfs_gettype(struct vnode *node)
{
    struct initramfs_inode *inode = node->internal;
    return inode->type;
}


long initramfs_getsize(struct vnode *vd)
{
    struct initramfs_inode *inode = vd->internal;
    return inode->datasize;
}
