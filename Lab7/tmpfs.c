#include "vfs.h"
#include "tmpfs.h"
#include "memory.h"
#include "shell.h"

struct file_operations tmpfs_file_operations = {tmpfs_write,tmpfs_read,tmpfs_open,tmpfs_close};
struct vnode_operations tmpfs_vnode_operations = {tmpfs_lookup,tmpfs_create,tmpfs_mkdir};

extern struct vnode * current_dir;

struct vnode * tmpfs_create_vnode(int type){
    struct vnode * node = malloc(sizeof(struct vnode));
    memset(node, 4096);
    //need to change to ops later
    node -> f_ops = 0;
    node -> v_ops = 0;
    struct tmpfs_node * tnode = malloc(sizeof(struct tmpfs_node));
    memset(tnode, 4096);
    tnode -> type = type;
    tnode -> data = allocate_page(4096);
    memset(tnode -> data, 4096);
    node -> internal = tnode;
    return node;
}

int tmpfs_mount(struct filesystem *fs, struct mount *mt){
    mt -> fs = fs;
    mt -> root = tmpfs_create_vnode(2); //2: mount
    strcpy("/root", ((struct tmpfs_node *)mt -> root -> internal) -> name);
    return 0;
}

int reg_tmpfs(){
    struct filesystem fs;
    fs.name = "tmpfs";
    fs.setup_mount = tmpfs_mount;
    return register_filesystem(&fs);
}

int tmpfs_write(struct file *file, const void *buf, size_t len){

}

int tmpfs_read(struct file *file, void *buf, size_t len){

}

int tmpfs_open(struct vnode *file_node, struct file **target){
    (*target) -> vnode = file_node;
    (*target) -> f_ops = file_node -> f_ops;
    (*target) -> f_pos = 0;
    return 0;
}

int tmpfs_close(struct file *file){
    free_page(file);
    return 0;
}

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){

}

int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name){

}

int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){

}

/*
struct file_operations {
  int (*write)(struct file* file, const void* buf, size_t len);
  int (*read)(struct file* file, void* buf, size_t len);
  int (*open)(struct vnode* file_node, struct file** target);
  int (*close)(struct file* file);
  //long (*lseek64)(struct file* file, long offset, int whence);
};

struct vnode_operations {
  int (*lookup)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  int (*create)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  int (*mkdir)(struct vnode* dir_node, struct vnode** target,
              const char* component_name);
};
*/