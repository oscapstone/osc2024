#include "vfs.h"
#include "uartfs.h"
#include "mini_uart.h"
#include "alloc.h"
#include "helper.h"

extern char* cpio_base;

struct file_operations uart_dev_file_operations = {uart_dev_write,uart_dev_read,uart_dev_open,uart_dev_close};
struct vnode_operations uart_dev_vnode_operations = {uart_dev_lookup,uart_dev_create,uart_dev_mkdir};

vnode * uart_dev_create_vnode(const char* name, char* data, int size){
    vnode* node = my_malloc(sizeof(vnode));
    memset(node, 0, sizeof(vnode));
    node -> f_ops = &uart_dev_file_operations;
    node -> v_ops = &uart_dev_vnode_operations;
    
	uart_dev_node* inode = my_malloc(sizeof(uart_dev_node)); 
    memset(inode, 0, sizeof(uart_dev_node));
    strcpy(name, inode -> name, strlen(name));
    node -> internal = inode;
    
	return node;
}

int uart_dev_mount(struct filesystem *_fs, struct mount *mt){
    mt -> fs = _fs;
    //set root
    const char * fname = "uart";
    mt -> root = uart_dev_create_vnode(fname, 0, 0);
    return 0;
}

int uart_dev_write(struct file *file, const char *buf, size_t len){
    for (int i = 0; i < len; i++) {
        uart_send(buf[i]);
    }
    return len;
}

int uart_dev_read(struct file *file, char *buf, size_t len){
    for (int i = 0; i < len; i++) {
    	buf[i] = uart_recv();
	}
    return len;
}

int uart_dev_open(struct vnode *file_node, struct file **target){
    (*target) -> vnode = file_node;
    (*target) -> f_ops = file_node -> f_ops;
    (*target) -> f_pos = 0;
	(*target) -> ref = 1;
    return 0;
}

int uart_dev_close(struct file *file){
	uart_printf ("shouldn't happen\r\n");
    return -1;
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
