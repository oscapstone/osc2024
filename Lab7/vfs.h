#include <stddef.h>
#define MAX_FS 64
#define MAX_PATH 255
#define MAX_DIR 255

//use vfs instruction for all, then it will call specific instruction by api (set by file systems)

/*
vnode: 
If mount, mounted file_system.
else, file or sub-directory(?) in internal (different in different file_system)
*/

/*
open: 
look for the file by name
simply open if exists, create then open if not exist
*/

/*
lookup:
look for sub vnode from current vnode
*/

/*
VFS_lookup:
given full path, iteratively update current vnode by running through each / until final vnode
*/

struct vnode {
  struct mount* mount;
  struct vnode_operations* v_ops;
  struct file_operations* f_ops;
  void* internal;
};

// file handle
struct file {
  struct vnode* vnode;
  size_t f_pos;  // RW position of this file handle
  struct file_operations* f_ops;
  int flags;
};

struct mount {
  struct vnode* root;
  struct filesystem* fs;
};

struct filesystem {
  const char* name;
  int (*setup_mount)(struct filesystem* fs, struct mount* mount);
};

struct file_operations {
  int (*write)(struct file* file, const void* buf, size_t len);
  int (*read)(struct file* file, void* buf, size_t len);
  int (*open)(struct vnode* file_node, struct file** target);
  int (*close)(struct file* file);
  long (*lseek64)(struct file* file, long offset, int whence);
};

struct vnode_operations {
  int (*lookup)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  int (*create)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  int (*mkdir)(struct vnode* dir_node, struct vnode** target,
              const char* component_name);
};

// bad implement
struct directory{
    const char * name;
    struct vnode* node;
};

void pwd();
void shell_with_dir();