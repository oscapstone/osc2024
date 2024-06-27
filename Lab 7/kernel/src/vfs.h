#ifndef __VFS_H__
#define __VFS_H__

#include "type.h"
#include "list.h"

#define FD_TABLE_SIZE           16
#define COMPONENT_SIZE          32

typedef struct vnode {
    struct mount *  mount;
    struct vnode_operations *   v_ops;
    struct file_operations *    f_ops;

    // internal
    struct vnode*   parent;
    list_head_t     children;
    list_head_t     self;
    uint32_t        child_num;
    byteptr_t       name;
    
    uint32_t        f_mode;

    byteptr_t       content;
    uint32_t        content_size;
} vnode_t;
typedef vnode_t * vnode_ptr;


typedef struct file {
    struct vnode *              vnode;
    struct file_operations *    f_ops;
    int32_t                     f_pos;  // RW position of this file handle
    int32_t                     flags;
} file_t;
typedef file_t * file_ptr;


typedef struct mount {
    struct vnode *              root;
    struct filesystem *         fs;
} mount_t;
typedef mount_t * mount_ptr;


typedef struct filesystem {
    const char *                name;
    int32_t (*setup_mount) (struct filesystem *fs, struct mount *mount);
    list_head_t                 list;
} filesystem_t;
typedef filesystem_t * fs_ptr; 


typedef struct file_operations {
    int32_t (*write)    (struct file *file, const void *buf, int32_t len);
    int32_t (*read)     (struct file *file, void *buf, int32_t len);
    int32_t (*open)     (struct vnode *file_node, struct file **target);
    int32_t (*close)    (struct file *file);
    int64_t (*lseek64)  (struct file *file, int64_t offset, int32_t whence);
} file_ops_t;
typedef file_ops_t * file_ops_ptr;


typedef struct vnode_operations {
    int32_t (*lookup)(struct vnode *dir_node, struct vnode **target, const char *component_name);
    int32_t (*create)(struct vnode *dir_node, struct vnode **target, const char *component_name);
    int32_t (*mkdir) (struct vnode *dir_node, struct vnode **target, const char *component_name);
} vnode_ops_t;
typedef vnode_ops_t * vnode_ops_ptr;


vnode_ptr vnode_create(const byteptr_t name, uint32_t flags);


void vfs_init();

int  vfs_register(struct filesystem *fs);
struct filesystem * vfs_get(const char *name);

int vfs_open(const char *pathname, int flags, struct file **target);
int vfs_close(struct file *file);
int vfs_write(struct file *file, const void *buf, int32_t len);
int vfs_read(struct file *file, void *buf, int32_t len);
int vfs_mkdir(const char *pathname);
int vfs_mount(const char *target, const char *filesystem);
int vfs_lookup(const char *pathname, struct vnode **target);
int vfs_chdir(const char *pathname);

void vfs_demo();


#endif