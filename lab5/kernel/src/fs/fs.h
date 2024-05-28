#pragma once

#include "base.h"
#include "utils/link_list.h"

#include "stat.h"

typedef struct _FS_VNODE {
    struct _FS_MOUNT* mount;
    struct _FS_VNODE_OPERATIONS* v_ops;
    struct _FS_FILE_OPERATIONS* f_ops;

    // internal
    LINK_LIST childs;
    size_t child_num;
    LINK_LIST self;
    struct _FS_VNODE* parent;
    char* name;
    U32 mode;
    void* content;
    size_t content_size;
}FS_VNODE;

typedef struct _FS_FILE {
    FS_VNODE* vnode;
    size_t pos;                 // r/w position of the file
    U32 flags;
}FS_FILE;

#define FS_FILE_FLAGS_NONE      0x0
#define FS_FILE_FLAGS_CREATE    0x1
#define FS_FILE_FLAGS_READ      0x2


#define O_CREAT                 FS_FILE_FLAGS_CREATE
#define O_READ                  FS_FILE_FLAGS_READ

typedef struct _FS_MOUNT {
    FS_VNODE* root;
    struct _FS_FILE_SYSTEM* fs;
}FS_MOUNT;

typedef struct _FS_FILE_SYSTEM {
    const char* name;
    int (*setup_mount) (struct _FS_FILE_SYSTEM* fs, FS_MOUNT* mount);   // function pointer of mounting this file system
    LINK_LIST list;                                             // entry to link list
}FS_FILE_SYSTEM;

typedef struct _FS_FILE_OPERATIONS {
    int (*write) (FS_FILE* file, const void* buf, size_t len);
    int (*read) (FS_FILE* file, void* buf, size_t len);
    int (*open) (FS_VNODE* file_node, FS_FILE** target);
    int (*close) (FS_FILE* file);
    long (*lseek64)(FS_FILE* file, long offset, int whence);
}FS_FILE_OPERATIONS;

typedef struct _FS_VNODE_OPERATIONS {
    int (*lookup) (FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
    int (*create) (FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
    int (*mkdir) (FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
}FS_VNODE_OPERATIONS;

typedef struct _FS_MANAGER {
    FS_MOUNT* rootfs;                       // where root is mount
    LINK_LIST filesystems;                  // list of file systems
}FS_MANAGER;


/**
 * initialize the rootfs
*/
// called by main (kernel)
void fs_init();

FS_VNODE* fs_get_root_node();

/**
 * @param node_name
 *      target node name to return
*/
int fs_find_node(const char* pathname, FS_VNODE** parent, FS_VNODE** target, char* node_name);
#define FS_FIND_NODE_HAS_PARENT_NO_TARGET           -10
#define FS_FIND_NODE_SUCCESS                        0

FS_VNODE *vnode_create(const char *name, U32 flags);

#define FS_OPEN_NO_PARENT_DIR                       -11
/**
 * @param flags
 *      FS_FILE_FLAGS
 * @return
 *      0 = success
*/
int vfs_open(const char* pathname, U32 flags, FS_FILE** target);
int vfs_close(FS_FILE* file);
int vfs_write(FS_FILE* file, const void* buf, size_t len);
int vfs_read(FS_FILE* file, void* buf, size_t len);

int vfs_lookup(const char* pathname, FS_VNODE** target);
int vfs_mkdir(const char* pathname);