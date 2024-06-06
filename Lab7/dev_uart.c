#include "vfs.h"
#include "memory.h"
#include "shell.h"
#include "utils.h"
#include "uart.h"
#include "dev_uart.h"

extern char* cpio_base;

struct file_operations uart_dev_file_operations = {uart_dev_write,uart_dev_read,uart_dev_open,uart_dev_close};
struct vnode_operations uart_dev_vnode_operations = {uart_dev_lookup,uart_dev_create,uart_dev_mkdir};

struct vnode * uart_dev_create_vnode(const char * name, char * data, int size, int type){
    struct vnode * node = allocate_page(4096);
    memset(node, 4096);
    node -> f_ops = &uart_dev_file_operations;
    node -> v_ops = &uart_dev_vnode_operations;
    struct uart_dev_node * inode = allocate_page(4096);
    memset(inode, 4096);
    inode -> type = type;
    strcpy(name, inode -> name);
    node -> internal = inode;
    return node;
}

int uart_dev_mount(struct filesystem *_fs, struct mount *mt){
    mt -> fs = _fs;
    //set root
    const char * fname = "uart";
    mt -> root = uart_dev_create_vnode(fname, 0, 0, 2); //1: directory, 2: mount
    strcpy("uart", ((struct uart_dev_node *)mt -> root -> internal) -> name);

    //create all entries for files
    struct uart_dev_node * inode = mt -> root -> internal;
    return 0;
}

int reg_uart_dev(){
    struct filesystem fs;
    fs.name = "uart";
    fs.setup_mount = uart_dev_mount;
    return register_filesystem(&fs);
}

int uart_dev_write(struct file *file, const char *buf, size_t len){
    for (int i = 0; i < len; i++) {
        uart_send(*(buf + i));
    }
    return len;
}

int uart_dev_read(struct file *file, char *buf, size_t len){
    for (int i = 0; i < len; i++) {
        *(buf + i) = uart_getc();
    }
    return len;
}

int uart_dev_open(struct vnode *file_node, struct file **target){
    (*target) -> vnode = file_node;
    (*target) -> f_ops = file_node -> f_ops;
    (*target) -> f_pos = 0;
    return 0;
}

int uart_dev_close(struct file *file){
    free_page(file);
    return 0;
}

int uart_dev_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){
    return -1;
}

int uart_dev_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
    return -1;
}

int uart_dev_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    return -1;
}