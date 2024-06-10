#include "initramfs.h"
#include "alloc.h"
#include "errno.h"
#include "io.h"
#include "string.h"
#include "lib.h"
#include "cpio.h"

struct vnode_operations *initramfs_v_ops;
struct file_operations *initramfs_f_ops;


static struct vnode* initramfs_create_vnode(const char *name, int type, struct vnode *parent);
static int isFileInSubDir(const char* pathname); 

int initramfs_setup_mount(struct filesystem* fs, struct mount **mount) // mount the filesystem to the mount point
{
    (*mount)->fs = fs;
    (*mount)->root = initramfs_create_vnode("/", DIR_NODE, NULL);
    
#ifndef QEMU
    cpio_newc_header* head = (void*)(uint64_t)CPIO_START_ADDR_FROM_DT;
#else
    cpio_newc_header* head = (void*)(uint64_t)CPIO_ADDR;
#endif
    // vfs_chdir("/initramfs");
    while(1)
    {
        char* pathname;
        char* filedata;
        char c_mode[5];
        strncpy(c_mode, head->c_mode, 5);
        int ret = cpio_newc_parser(&head, &pathname, &filedata);
        if(ret == 1)break;
        // printf("\r\n[INITRAMFS INFO] Mount Archive: "); printf(pathname); printf("\t, cmode: "); printf(c_mode);
        struct initramfs_internal *root_internal = (struct initramfs_internal*)(*mount)->root->internal;

        if(!strcmp(c_mode, "00008")){ // file
            if(!strcmp(pathname, ".") || !strcmp(pathname, "..") || isFileInSubDir(pathname))continue;
            struct vnode *filevnode = initramfs_create_vnode(pathname, FILE_NODE, (*mount)->root);
            struct initramfs_internal *fileinode = (struct initramfs_internal*)filevnode->internal;
            fileinode->data = filedata;
            fileinode->size = 0;
            root_internal->children[root_internal->size++] = filevnode;
            printf("\r\n[INITRAMFS INFO] Mount Archive: "); printf(pathname); printf("\t, cmode: "); printf(c_mode);
        }
        else if(!strcmp(c_mode, "00004")){ // directory
            // cannot create directory in initramfs
        }
        else{
            printf("\r\n[ERROR] Unsupported file type");
            return -1;
        }
    }
    printf("\r\n[INITRAMFS INFO] Mounting complete");

    return 0;
}

static struct vnode* initramfs_create_vnode(const char *name, int type, struct vnode *parent)
{
    struct initramfs_internal *internal = (struct initramfs_internal*)dynamic_alloc(sizeof(struct initramfs_internal));

    if(sizeof(name) > COMPONENT_SIZE){
        printf("\r\n[ERROR] File name too long");
        return NULL;
    }
    internal->name = (char*)dynamic_alloc(COMPONENT_SIZE);
    strcpy(internal->name, name);
    internal->size = 0;
    if(type == FILE_NODE){
        internal->data = (char*)dynamic_alloc(INITRAMFS_MAX_FILE);
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
    vnode->v_ops = initramfs_v_ops;
    vnode->f_ops = initramfs_f_ops;
    vnode->internal = internal;
    return vnode;
}

int initramfs_register()
{
    // check if tmpfs already registered
    int i=0;
    for(; i<MAX_FILESYSTEM; i++)
    {
        if(global_fs[i].name == NULL)break;
    }
    if(i == MAX_FILESYSTEM)
    {
        printf("\r\n[ERROR] Maximum number of filesystems reached");
        return -1;
    }
    global_fs[i].setup_mount = initramfs_setup_mount;
    global_fs[i].name = "initramfs";


    initramfs_v_ops = (struct vnode_operations*)dynamic_alloc(sizeof(struct vnode_operations));
    initramfs_f_ops = (struct file_operations*)dynamic_alloc(sizeof(struct file_operations));

    initramfs_v_ops->lookup = initramfs_lookup;
    initramfs_v_ops->list = initramfs_list;
    initramfs_v_ops->create = initramfs_create;
    initramfs_v_ops->mkdir = initramfs_mkdir;

    initramfs_f_ops->open = initramfs_open;
    initramfs_f_ops->read = initramfs_read;
    initramfs_f_ops->write = initramfs_write;
    initramfs_f_ops->close = initramfs_close;

    return 0;
}

int initramfs_list(struct vnode* dir_node)
{
    // 1. List all the files in the directory
    // 2. Return error code if fails
    
    printf("\r\n[INITRAMFS LIST]");
    struct initramfs_internal *internal = (struct initramfs_internal*)dir_node->internal;
    
    if(internal->type == FILE_NODE){
        printf("\r\n[ERROR] Cannot list in a file");
        return LERROR;
    }

    
    for(int i=0; i<internal->size; i++){
        struct initramfs_internal *child_internal = (struct initramfs_internal*)internal->children[i]->internal;
        printf("\r\n"); printf(child_internal->name); printf("\t"); printf(child_internal->type == FILE_NODE ? "FILE" : "DIR");
    }
    return 0;
}

int initramfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name)
{
    // 1. Lookup the vnode in the directory
    // 2. Return error code if fails
    struct initramfs_internal *internal = (struct initramfs_internal*)dir_node->internal;
    if(internal->type == FILE_NODE){
        printf("\r\n[ERROR] Cannot lookup in a file");
        return LERROR;
    }

    for(int i=0; i<ENTRIES_PER_DIR; i++){
        struct initramfs_internal *child_internal = (struct initramfs_internal*)internal->children[i]->internal;
        if(internal->children[i] != NULL && !strcmp(child_internal->name, component_name)){
            *target = internal->children[i];
            return 0;
        }
    }
    // printf("[ERROR] File not found\n");
    return LERROR;
}

int initramfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name)
{
    // Alawys return -1 because we are not implementing mkdir in initramfs
    return -1;
}

int initramfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name)
{
    // Alawys return -1 because we are not implementing create in initramfs
    return -1;
}

int initramfs_close(struct file* file) 
{
    // 1. release the file handle
    // 2. Return error code if fails
    if(file == NULL){
        printf("\r\n[ERROR] File pointer cannot be NULL");
        return CERROR;
    }
    return 0; // vfs_close will free the file pointer 
}

int initramfs_open(struct vnode* file_node, struct file** target)
{
    // 1. Allocate file handle
    // 2. Return error code if fails
    struct file *file = (struct file*)dynamic_alloc(sizeof(struct file));
    file->vnode = file_node;
    file->f_pos = 0;
    *target = file;
    return 0;
}

int initramfs_read(struct file* file, void* buf, size_t len)
{
    struct initramfs_internal *internal = (struct initramfs_internal*)file->vnode->internal;
    if(internal->type != FILE_NODE)
    {
        printf("\r\n[ERROR] Not a file");
        return -1;
    }
    char *dst = (char*)buf;
    char *src = &((char *)internal->data)[file->f_pos];
    int i = 0;
    for (; i<len && i < internal->size; i++) {
        dst[i] = src[i];
    }
    return i;
}

int initramfs_write(struct file* file, const void* buf, size_t len)
{
    //cannot write on initframfs
    return -1;
}



static int isFileInSubDir(const char* pathname)
{
    for(int i=0; i<strlen(pathname); i++)
    {
        if(pathname[i] == '/')return 1;
    }
    return 0;
}