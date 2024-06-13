#include "kernel/tmpfs.h"

struct file_operations tmpfs_f_ops = {tmpfs_write, tmpfs_read, tmpfs_open, tmpfs_close, vfs_lseek64, tmpfs_getsize};
struct vnode_operations tmpfs_v_ops = {tmpfs_lookup, tmpfs_create, tmpfs_mkdir};

int tmpfs_register(){
    struct filesystem fs;
    fs.name = "tmpfs";
    fs.setup_mount = tmpfs_setup_mount;
    // return the index of the filesystem
    return register_filesystem(&fs);
}

struct vnode* tmpfs_create_vnode(struct mount* mount, enum node_type type){
    struct vnode* vnode = (struct vnode*)pool_alloc(4096);
    struct tmpfs_inode* inode = (struct tmpfs_inode*)pool_alloc(4096);

    vnode->mount = mount;
    vnode->v_ops = &tmpfs_v_ops;
    vnode->f_ops = &tmpfs_f_ops;

    memset(inode, 0, sizeof(struct tmpfs_inode));
    inode->type = type;
    inode->data = (char*)pool_alloc(MAX_FILE_SIZE);
    vnode->internal = inode;
    return vnode;
}

int tmpfs_setup_mount(struct filesystem *fs, struct mount *mount){
    mount->fs = fs;
    // we use 0 as the "mount" variable belonged to directory vnode
    mount->root = tmpfs_create_vnode(0, dir_t);
    return 0;
}

int tmpfs_write(struct file *file, const void *buf, my_uint64_t len){
    struct tmpfs_inode* inode = (struct tmpfs_inode*)file->vnode->internal;

    string_copy(inode->data + file->f_pos, (char*)buf);
    file->f_pos += len;
    // update the size of the file
    if(file->f_pos > inode->data_size)
        inode->data_size = file->f_pos;

    return len;    
}

int tmpfs_read(struct file *file, void *buf, my_uint64_t len){
    struct tmpfs_inode* inode = (struct tmpfs_inode*)file->vnode->internal;
    
    if(file->f_pos + len > inode->data_size){
        len = inode->data_size - file->f_pos;
        string_copy_n(buf, inode->data + file->f_pos, len);
    }
    else
        string_copy_n(buf, inode->data + file->f_pos, len);

    file->f_pos += len;

    return len;
}

int tmpfs_open(struct vnode *file_node, struct file **target){
    (*target)->vnode = file_node;
    (*target)->f_pos = 0;
    (*target)->f_ops = file_node->f_ops;

    return 0;
}

int tmpfs_close(struct file *file){
    pool_free(file);
    return 0;
}

long tmpfs_getsize(struct vnode *vd){
    struct tmpfs_inode* inode = (struct tmpfs_inode*)vd->internal;
    return inode->data_size;
}
// lookup a vnode in dir_node
int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){
    struct tmpfs_inode* dir_inode = (struct tmpfs_inode*)dir_node->internal;

    for(int i = 0; i < MAX_DIR_ENTRY; i++){
        struct vnode* vnode = dir_inode->entry[i];
        if(vnode == 0)
            break;

        struct tmpfs_inode* inode = (struct tmpfs_inode*)vnode->internal;
        if(string_comp(inode->name, component_name) == 0){
            *target = vnode;
            return 0;
        }
    }

    uart_puts("tmpfs Lookup Error: Cannot find the vnode\n");
    return -1;
}

int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
    struct tmpfs_inode* inode = (struct tmpfs_inode*)dir_node->internal;
    
    if(inode->type != dir_t){
        uart_puts("tmpfs Create Error: Not a directory\n");
        return -1;
    }

    int entry_index;
    
    for(entry_index = 0; entry_index < MAX_DIR_ENTRY; entry_index++){
        // find an empty entry
        if(inode->entry[entry_index] == 0)
            break;

        if(string_comp(((struct tmpfs_inode*)(inode->entry[entry_index]->internal))->name, component_name) == 0){
            uart_puts("tmpfs Create Error: File already exist\n");
            return -1;
        }
    }

    if(entry_index == MAX_DIR_ENTRY){
        uart_puts("tmpfs Create Error: Directory is full\n");
        return -1;
    }

    if(string_len(component_name) > MAX_FILE_NAME_LEN){
        uart_puts("tmpfs Create Error: File name too long\n");
        return -1;
    }
    // create a vnode for the new file
    inode->entry[entry_index] = tmpfs_create_vnode(0, file_t);
    string_copy(((struct tmpfs_inode*)(inode->entry[entry_index]->internal))->name, (char*)component_name);

    *target = inode->entry[entry_index];
    return 0;
}

int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    struct tmpfs_inode* inode = (struct tmpfs_inode*)dir_node->internal;
    
    if(inode->type != dir_t){
        uart_puts("tmpfs Mkdir Error: Not a directory\n");
        return -1;
    }

    int entry_index;
    
    for(entry_index = 0; entry_index < MAX_DIR_ENTRY; entry_index++){
        // find an empty entry
        if(inode->entry[entry_index] == 0)
            break;

        if(string_comp(((struct tmpfs_inode*)(inode->entry[entry_index]->internal))->name, component_name) == 0){
            uart_puts("tmpfs Mkdir Error: Directory already exist\n");
            return -1;
        }
    }

    if(entry_index == MAX_DIR_ENTRY){
        uart_puts("tmpfs Mkdir Error: Directory is full\n");
        return -1;
    }

    if(string_len(component_name) > MAX_FILE_NAME_LEN){
        uart_puts("tmpfs Mkdir Error: Directory name too long\n");
        return -1;
    }
    // create a vnode for the new directory
    inode->entry[entry_index] = tmpfs_create_vnode(0, dir_t);
    string_copy(((struct tmpfs_inode*)(inode->entry[entry_index]->internal))->name, (char*)component_name);

    *target = inode->entry[entry_index];
    return 0;
}