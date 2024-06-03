#pragma once

#include <stdarg.h>

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

#define FS_FILE_FLAGS_READ      0x0
#define FS_FILE_FLAGS_WRITE     0x1
#define FS_FILE_FLAGS_RDWR      0x2
#define FS_FILE_FLAGS_CREATE    0x40


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
    int (*ioctl)(FS_FILE *file, unsigned long request, ...);
}FS_FILE_OPERATIONS;

typedef struct _FS_VNODE_OPERATIONS {
    int (*lookup) (FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
    int (*create) (FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
    int (*mkdir) (FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
}FS_VNODE_OPERATIONS;

typedef struct _FS_MANAGER {
    FS_MOUNT rootfs;                       // where root is mount
    LINK_LIST filesystems;                  // list of file systems
}FS_MANAGER;


/**
 * initialize the rootfs
*/
// called by main (kernel)
void fs_init();
int fs_register(FS_FILE_SYSTEM* fs);
FS_FILE_SYSTEM* fs_get(const char *name);

FS_VNODE* fs_get_root_node();

#define FS_MAX_NAME_SIZE        256

/**
 * @param cwd
 *      current working directory
 * @param pathname
 *      path to search
 * @param parent
 *      parent directory to return
 * @param target
 *      target to return
 * @param node_name
 *      final node name to return whatever it can be find
*/
int fs_find_node(FS_VNODE* cwd, const char* pathname, FS_VNODE** parent, FS_VNODE** target, char* node_name);
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
int vfs_open(FS_VNODE* cwd, const char* pathname, U32 flags, FS_FILE** target);
int vfs_close(FS_FILE* file);
int vfs_write(FS_FILE* file, const void* buf, size_t len);
int vfs_read(FS_FILE* file, void* buf, size_t len);
#define SEEK_SET 0
long vfs_lseek64(FS_FILE* file, long offset, int whence);

int vfs_lookup(FS_VNODE* cwd, const char* pathname, FS_VNODE** target);
int vfs_mkdir(FS_VNODE* cwd, const char* pathname);