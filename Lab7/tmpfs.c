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
    node -> f_ops = &tmpfs_file_operations;
    node -> v_ops = &tmpfs_vnode_operations;
    struct tmpfs_node * tnode = malloc(sizeof(struct tmpfs_node));
    memset(tnode, 4096);
    tnode -> type = type;
    tnode -> data = allocate_page(4096);
    tnode -> size = 0;
    memset(tnode -> data, 4096);
    node -> internal = tnode;
    return node;
}

int tmpfs_mount(struct filesystem *fs, struct mount *mt){
    mt -> fs = fs;
    mt -> root = tmpfs_create_vnode(2); //1: directory, 2: mount
    strcpy("/", ((struct tmpfs_node *)mt -> root -> internal) -> name);
    return 0;
}

int reg_tmpfs(){
    struct filesystem fs;
    fs.name = "tmpfs";
    fs.setup_mount = tmpfs_mount;
    return register_filesystem(&fs);
}

int tmpfs_write(struct file *file, const void *buf, size_t len){
    struct tmpfs_node * internal = file -> vnode -> internal;
    for(int i=0; i<len; i++){
        (internal -> data)[file -> f_pos + i] = ((char* )buf)[i];
    }
    file -> f_pos += len;
    if(internal -> size < file -> f_pos)
        internal -> size = f_pos;
    return len;
}

int tmpfs_read(struct file *file, void *buf, size_t len){
    struct tmpfs_node * internal = file -> vnode -> internal;
    for(int i=0; i<len;i++){
        if(internal -> data [i + file -> f_pos] == 0){
            file -> f_pos += i;
            return i;
        }
        ((char *)buf)[i] = internal -> data[i + file -> f_pos];
    }
    file -> f_pos += len;
    return len;
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
    int idx = 0;
    struct tmpfs_node * internal = dir_node -> internal;
    while(internal -> entry[idx]){
        struct tmpfs_node * entry_node = internal -> entry[idx] -> internal;
        if(strcmp(entry_node -> name, component_name) == 0){
            *target = internal -> entry[idx];
            return 0;
        }
        idx++;
    }
    return -1;
}

int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
    struct tmpfs_node * internal = dir_node -> internal;
    if(internal -> type != 1 && internal -> type != 2){//2: mount
        uart_puts("NOT A DIRECTORY!\n\r");
        return -1;
    }

    int idx = -1;
    for(int i=0; i<MAX_ENTRY; i++){
        if(internal -> entry[i] == 0){
            idx = i;
            break;
        }
        struct tmpfs_node* infile = internal -> entry[i] -> internal;
        if(strcmp(infile -> name, component_name) == 0){
            uart_puts("FILE EXISTED!!!\n\r");
            return -1;
        }
    }

    if(idx == -1){
        uart_puts("DIRECTORY FULL!!!\n\r");
    }

    struct vnode * node = tmpfs_create_vnode(3); //3: file
    struct tmpfs_node * inode = node -> internal;
    strcpy(component_name, inode -> name);
    internal -> entry[idx] = node;
    *target = node;
    return 0;
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