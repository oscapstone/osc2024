#include "tmpfs.h"
#include "alloc.h"
#include "errno.h"
#include "io.h"
#include "string.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

struct file_operations *tmpfs_f_ops; 
struct vnode_operations *tmpfs_v_ops;

static struct vnode* tmpfs_create_vnode(const char *name, int type, struct vnode *parent);

int tmpfs_setup_mount(struct filesystem* fs, struct mount *mount) // mount the filesystem to the mount point
{
    mount->fs = fs;
    mount->root = tmpfs_create_vnode("/", DIR_NODE, NULL);
    return 0;
}

static struct vnode* tmpfs_create_vnode(const char *name, int type, struct vnode *parent)
{
    struct tmpfs_internal *internal = (struct tmpfs_internal*)dynamic_alloc(sizeof(struct tmpfs_internal));

    if(sizeof(name) > COMPONENT_SIZE){
        printf("[ERROR] File name too long\n");
        return NULL;
    }
    internal->name = (char*)dynamic_alloc(COMPONENT_SIZE);
    strcpy(internal->name, name);
    internal->size = 0;
    if(type == FILE_NODE){
        internal->data = (char*)balloc(TMPFS_MAX_FILE);
    }
    else{
        internal->data = NULL;
    }
    internal->type = type;
    // internal->parent = parent;
    // internal->next = NULL;
    for(int i=0; i<ENTRIES_PER_DIR; i++)
    {
        internal->children[i] = NULL;
    }
    struct vnode *vnode = (struct vnode*)dynamic_alloc(sizeof(struct vnode));
    vnode->parent = parent;
    vnode->mount = NULL;
    vnode->v_ops = tmpfs_v_ops;
    vnode->f_ops = tmpfs_f_ops;
    vnode->internal = internal;
    return vnode;
}

int tmpfs_register()
{
    // check if tmpfs already registered
    int i=0;
    for(; i<MAX_FILESYSTEM; i++)
    {
        if(global_fs[i].name == NULL)break;
        if(!strcmp(global_fs[i].name, "tmpfs")){
            printf("[ERROR] tmpfs already registered\n");
            return -1;
        }
    }
    if(i == MAX_FILESYSTEM){
        printf("[ERROR] No space for new filesystem\n");
        return -1;
    }

    global_fs[i].name = "tmpfs";
    global_fs[i].setup_mount = tmpfs_setup_mount;

    tmpfs_f_ops = (struct file_operations*)dynamic_alloc(sizeof(struct file_operations));
    tmpfs_v_ops = (struct vnode_operations*)dynamic_alloc(sizeof(struct vnode_operations));

    tmpfs_f_ops->write = tmpfs_write;
    tmpfs_f_ops->read = tmpfs_read;
    tmpfs_f_ops->open = tmpfs_open;
    tmpfs_f_ops->close = tmpfs_close;

    tmpfs_v_ops->lookup = tmpfs_lookup;
    tmpfs_v_ops->create = tmpfs_create;
    tmpfs_v_ops->mkdir = tmpfs_mkdir;
    tmpfs_v_ops->list = tmpfs_list;

    return 0;
}

int tmpfs_write(struct file* file, const void* buf, size_t len)
{
    // Given the file handle, VFS calls the corresponding write 
    // method to write the file starting from f_pos, then updates 
    // f_pos and size after write. (or not if it’s a special file) 
    // Returns size written or error code on error.

    // 1. write len byte from buf to the opened file.
    // 2. return written size or error code if an error occurs.
    struct vnode *vnode = file->vnode;
    struct tmpfs_internal *internal = (struct tmpfs_internal*)vnode->internal;
    if(internal->type == DIR_NODE){
        printf("[ERROR] Cannot write to a directory\n");
        return WERROR;
    }

    if(file->f_pos + len > TMPFS_MAX_FILE){
        printf("[ERROR] File size exceeded\n");
        return WERROR;
    }
    
    // memory copy
    for(int i=0; i<len; i++){
        internal->data[file->f_pos + i] = ((char*)buf)[i];
    }
    file->f_pos += len;
    internal->size += len;
    return len;
}

int tmpfs_read(struct file* file, void* buf, size_t len)
{
    // Given the file handle, VFS calls the corresponding read method 
    // to read the file starting from f_pos, then updates f_pos after read. 
    // (or not if it’s a special file)

    // 1. read min(len, readable size) byte to buf from the opened file.
    // 2. block if nothing to read for FIFO type
    // 2. return read size or error code if an error occurs.

    struct vnode *vnode = file->vnode;
    struct tmpfs_internal *internal = (struct tmpfs_internal*)vnode->internal;
    if(internal->type == DIR_NODE){
        printf("[ERROR] Cannot read a directory\n");
        return RERROR;
    }

    size_t readable_size = min(len, internal->size - file->f_pos);

    // memory copy
    for(int i=0; i<readable_size; i++){
        ((char*)buf)[i] = internal->data[file->f_pos + i];
    }
    file->f_pos += readable_size;
    return readable_size;
}

int tmpfs_open(struct vnode* file_node, struct file** target) 
{
    return 0; // the steps for checking the file is implemented in vfs_open
}

int tmpfs_close(struct file* file) 
{
    // 1. release the file handle
    // 2. Return error code if fails
    if(file == NULL){
        printf("[ERROR] File pointer cannot be NULL\n");
        return CERROR;
    }
    return 0; // vfs_close will free the file pointer 
}

int tmpfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name)
{
    // create an regular file on underlying file system, 
    // should fail if file exist. Then passes the file’s vnode back to VFS.

    // 1. Create a new file in the directory
    // 2. Return error code if fails
    struct tmpfs_internal *internal = (struct tmpfs_internal*)dir_node->internal;
    if(internal->type == FILE_NODE){
        printf("[ERROR] Cannot create file in a file\n");
        return CERROR;
    }

    if(strlen(component_name) > COMPONENT_SIZE){
        printf("[ERROR] File name too long\n");
        return CERROR;
    }

    // for(int i=0; i<ENTRIES_PER_DIR; i++){
    //     if(internal->children[i] == NULL){
    //         internal->children[i] = tmpfs_create_vnode(component_name, FILE_NODE, dir_node);
    //         *target = internal->children[i];
    //         return 0;
    //     }
    // }
    if(internal->size + 1 > ENTRIES_PER_DIR){
        printf("[ERROR] No space for new file\n");
        return CERROR;
    }
    internal->children[internal->size] = tmpfs_create_vnode(component_name, FILE_NODE, dir_node);
    *target = internal->children[internal->size];
    internal->size++;
    return 0;
}

int tmpfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name)
{
    // 1. Create a new directory in the directory
    // 2. Return error code if fails
    printf("\r\ntmpfs mkdir");
    struct tmpfs_internal *internal = (struct tmpfs_internal*)dir_node->internal;
    if(internal->type == FILE_NODE){
        printf("[ERROR] Cannot create directory in a file\n");
        return CERROR;
    }

    if(strlen(component_name) > COMPONENT_SIZE){
        printf("[ERROR] Directory name too long\n");
        return CERROR;
    }

    // for(int i=0; i<ENTRIES_PER_DIR; i++){
    //     if(internal->children[i] == NULL){
    //         internal->children[i] = tmpfs_create_vnode(component_name, DIR_NODE, dir_node);
    //         *target = internal->children[i];
    //         return 0;
    //     }
    // }
    if(internal->size + 1 > ENTRIES_PER_DIR){
        printf("[ERROR] No space for new directory\n");
        return CERROR;
    }
    internal->children[internal->size] = tmpfs_create_vnode(component_name, DIR_NODE, dir_node);
    *target = internal->children[internal->size];
    internal->size++;
    return 0;
}

int tmpfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name)
{
    // 1. Lookup the vnode in the directory
    // 2. Return error code if fails
    struct tmpfs_internal *internal = (struct tmpfs_internal*)dir_node->internal;
    if(internal->type == FILE_NODE){
        printf("[ERROR] Cannot lookup in a file\n");
        return LERROR;
    }

    for(int i=0; i<ENTRIES_PER_DIR; i++){
        struct tmpfs_internal *child_internal = (struct tmpfs_internal*)internal->children[i]->internal;
        if(internal->children[i] != NULL && !strcmp(child_internal->name, component_name)){
            *target = internal->children[i];
            return 0;
        }
    }
    // printf("[ERROR] File not found\n");
    return LERROR;
}

int tmpfs_list(struct vnode* dir_node)
{
    // 1. List all the files in the directory
    // 2. Return error code if fails
    
    struct tmpfs_internal *internal = (struct tmpfs_internal*)dir_node->internal;
    
    if(internal->type == FILE_NODE){
        printf("[ERROR] Cannot list in a file\n");
        return LERROR;
    }

    for(int i=0; i<internal->size; i++){
        struct tmpfs_internal *child_internal = (struct tmpfs_internal*)internal->children[i]->internal;
        printf("\r\n"); printf(child_internal->name); printf("\t"); printf(child_internal->type == FILE_NODE ? "FILE" : "DIR");
    }
    return 0;
}