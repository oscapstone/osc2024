#include "vfs.h"
#include "tmpfs.h"
#include "memory.h"
#include "shell.h"

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
