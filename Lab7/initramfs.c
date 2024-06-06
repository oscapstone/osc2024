#include "vfs.h"
#include "tmpfs.h"
#include "memory.h"
#include "shell.h"
#include "utils.h"
#include "initramfs.h"

extern char* cpio_base;

struct file_operations initramfs_file_operations = {initramfs_write,initramfs_read,initramfs_open,initramfs_close};
struct vnode_operations initramfs_vnode_operations = {initramfs_lookup,initramfs_create,initramfs_mkdir};

struct vnode * initramfs_create_vnode(const char * name, char * data, int size, int type){
    uart_puts("Create initramfs file name: ");
    struct vnode * node = allocate_page(4096);
    memset(node, 4096);
    node -> f_ops = &initramfs_file_operations;
    node -> v_ops = &initramfs_vnode_operations;
    struct initramfs_node * inode = allocate_page(4096);
    memset(inode, 4096);
    inode -> type = type;
    strcpy(name, inode -> name);
    inode -> data = data;
    inode -> size = size;
    node -> internal = inode;
    uart_puts(((struct initramfs_node *) node -> internal) -> name);
    newline();
    return node;
}

int initramfs_mount(struct filesystem *_fs, struct mount *mt){
    //initialize all entries
    mt -> fs = _fs;
    //set root
    const char * fname = "initramfs";
    mt -> root = initramfs_create_vnode(fname, 0, 0, 3); //1: directory, 2: mount
    strcpy("initramfs", ((struct initramfs_node *)mt -> root -> internal) -> name);

    //create all entries for files
    struct initramfs_node * inode = mt -> root -> internal;
    struct cpio_newc_header *fs = (struct cpio_newc_header *)cpio_base;
    char *current = (char *)cpio_base;
    int idx = 0;

    while(1){
        fs = (struct cpio_newc_header *)current;
        int name_size = hex_to_int(fs->c_namesize, 8);
        int file_size = hex_to_int(fs->c_filesize, 8);
        current += 110; // size of cpio_newc_header
        
        if (strcmp(current, "TRAILER!!!") == 0)
            break;

        char * name = current;
        current += name_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
        
        char * data = current;
        if(strcmp(name, ".") != 0){
            inode -> entry[idx] = initramfs_create_vnode(name, data, file_size, 1);
            idx++;
        }
            
        current += file_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
    }

    return 0;
}

int reg_initramfs(){
    struct filesystem fs;
    fs.name = "initramfs";
    fs.setup_mount = initramfs_mount;
    return register_filesystem(&fs);
}

int initramfs_write(struct file *file, const void *buf, size_t len){
    return -1;
}

int initramfs_read(struct file *file, void *buf, size_t len){
    struct initramfs_node * internal = file -> vnode -> internal;
    if(file -> f_pos + len > internal -> size)
        len = internal -> size - file -> f_pos;
    for(int i=0; i<len;i++){
        ((char *)buf)[i] = internal -> data[i + file -> f_pos];
    }
    file -> f_pos += len;
    return len;
}

int initramfs_open(struct vnode *file_node, struct file **target){
    (*target) -> vnode = file_node;
    (*target) -> f_ops = file_node -> f_ops;
    (*target) -> f_pos = 0;
    return 0;
}

int initramfs_close(struct file *file){
    free_page(file);
    return 0;
}

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){
    int idx = 0;
    struct initramfs_node * internal = dir_node -> internal;
    while(internal -> entry[idx]){
        struct initramfs_node * entry_node = internal -> entry[idx] -> internal;
        if(strcmp(entry_node -> name, component_name) == 0){
            *target = internal -> entry[idx];
            return 0;
        }
        idx++;
    }
    return -1;
}

int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
    return -1;
}

int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    return -1;
}