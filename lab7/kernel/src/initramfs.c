#include "initramfs.h"
#include "alloc.h"
#include "vfs.h"
#include "mini_uart.h"
#include "helper.h"
#include "fdt.h"
#include "cpio.h"

extern char* _cpio_file;

struct file_operations initramfs_file_operations = {initramfs_write,initramfs_read,initramfs_open,initramfs_close};
struct vnode_operations initramfs_vnode_operations = {initramfs_lookup,initramfs_create,initramfs_mkdir};

struct vnode * initramfs_create_vnode(const char* name, char * data, int size){
    vnode* node = my_malloc(sizeof(vnode));
    memset(node, 0, sizeof(vnode));
    node -> f_ops = &initramfs_file_operations;
    node -> v_ops = &initramfs_vnode_operations;
    initramfs_node* inode = my_malloc(sizeof(initramfs_node));
    memset(inode, 0, sizeof(inode));
	strcpy(name, inode -> name, strlen(name));
    inode -> data = data;
    inode -> size = size;
    node -> internal = inode;
    uart_printf ("created %s\r\n", ((struct initramfs_node *) node -> internal) -> name);
    return node;
}

int initramfs_mount(struct filesystem *_fs, struct mount *mt){
    //initialize all entries
    mt -> fs = _fs;
    //set root
    const char * fname = "initramfs";
    mt -> root = initramfs_create_vnode(fname, 0, 0);

    //create all entries for files
    struct initramfs_node* inode = mt -> root -> internal;
    struct cpio_newc_header *fs = (struct cpio_newc_header *)_cpio_file;
    char *current = (char *)_cpio_file;
    int idx = 0;

    while(1){
        fs = (struct cpio_newc_header *)current;
        int name_size = hex_to_bin(fs->c_namesize);
        int file_size = hex_to_bin(fs->c_filesize);
        current += 110; // size of cpio_newc_header

        if (same(current, "TRAILER!!!"))
            break;

        char * name = current;
        current += name_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);

        char * data = current;
        if(!same(name, ".")){
            inode -> entry[idx] = initramfs_create_vnode(name, data, file_size);
            idx++;
        }

        current += file_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
    }

    return 0;
}


int initramfs_write(struct file *file, const void *buf, size_t len){
    uart_printf ("[INITRAMFS]Trying to write to initramfs\r\n");
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
	(*target) -> ref = 1;
    return 0;
}

int initramfs_close(struct file *file){
	// uart_printf ("Shouldn't happen\r\n");
    my_free(file);
    return -1;
}

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){
    int idx = 0;
    struct initramfs_node * internal = dir_node -> internal;
    while(internal -> entry[idx]){
        struct initramfs_node * entry_node = internal -> entry[idx] -> internal;
        if(same(entry_node -> name, component_name)){
            *target = internal -> entry[idx];
            return 0;
        }
        idx++;
    }
    return -1;
}

int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
    uart_printf ("[INITRAMFS]Trying to create to initramfs\r\n");
    return -1;
}

int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    uart_printf ("[INITRAMFS]Trying to mkdir to initramfs\r\n");
    return -1;
}
