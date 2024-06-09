#include "sd_driver.h"
#include "vfs.h"
#include "tmpfs.h"
#include "memory.h"
#include "shell.h"
#include "utils.h"

extern char* cpio_base;

struct file_operations fat32_file_operations = {fat32_write,fat32_read,fat32_open,fat32_close};
struct vnode_operations fat32_vnode_operations = {fat32_lookup,fat32_create,fat32_mkdir};

struct vnode * fat32_create_vnode(const char * name, char * data, int size, int type){
    uart_puts("Create fat32 file name: ");
    struct vnode * node = allocate_page(4096);
    memset(node, 4096);
    node -> f_ops = &fat32_file_operations;
    node -> v_ops = &fat32_vnode_operations;
    struct fat32_node * inode = allocate_page(4096);
    memset(inode, 4096);
    inode -> type = type;
    strcpy(name, inode -> name);
    inode -> data = data;
    inode -> size = size;
    node -> internal = inode;
    uart_puts(((struct fat32_node *) node -> internal) -> name);
    newline();
    return node;
}

int fat32_mount(struct filesystem *_fs, struct mount *mt){
    //initialize all entries
    mt -> fs = _fs;
    //set root
    const char * fname = "fat32";
    mt -> root = fat32_create_vnode(fname, 0, 0, 3); //1: directory, 2: mount
    strcpy("fat32", ((struct fat32_node *)mt -> root -> internal) -> name);

    //create all entries for files
    struct fat32_node * inode = mt -> root -> internal;
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
            inode -> entry[idx] = fat32_create_vnode(name, data, file_size, 1);
            idx++;
        }
            
        current += file_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
    }

    return 0;
}

int reg_fat32(){
    struct filesystem fs;
    fs.name = "fat32";
    fs.setup_mount = fat32_mount;
    return register_filesystem(&fs);
}

int fat32_write(struct file *file, const void *buf, size_t len){
    return -1;
}

int fat32_read(struct file *file, void *buf, size_t len){
    struct fat32_node * internal = file -> vnode -> internal;
    if(file -> f_pos + len > internal -> size)
        len = internal -> size - file -> f_pos;
    for(int i=0; i<len;i++){
        ((char *)buf)[i] = internal -> data[i + file -> f_pos];
    }
    file -> f_pos += len;
    return len;
}

int fat32_open(struct vnode *file_node, struct file **target){
    (*target) -> vnode = file_node;
    (*target) -> f_ops = file_node -> f_ops;
    (*target) -> f_pos = 0;
    return 0;
}

int fat32_close(struct file *file){
    free_page(file);
    return 0;
}

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){
    int idx = 0;
    struct fat32_node * internal = dir_node -> internal;
    while(internal -> entry[idx]){
        struct fat32_node * entry_node = internal -> entry[idx] -> internal;
        if(strcmp(entry_node -> name, component_name) == 0){
            *target = internal -> entry[idx];
            return 0;
        }
        idx++;
    }
    return -1;
}

int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
    return -1;
}

int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    return -1;
}