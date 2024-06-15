#ifndef TMPFS_H
#define TMPFS_H

#include "mini_uart.h"
#include "vfs.h"
#include "helper.h"
#include "alloc.h"

#define MAX_DATA_SIZE 4096
#define MAX_NAME_SIZE 15
#define MAX_DIR_SIZE 16

typedef struct tmpfs_node {
	char data[MAX_DATA_SIZE];
	char name[MAX_NAME_SIZE];
	vnode* entry[MAX_DIR_SIZE];
	int type; // 1: directory, 2: file
	int size;
} tmpfs_node;

int tmpfs_write(struct file *file, const void *buf, size_t len);
int tmpfs_read(struct file *file, void *buf, size_t len);
int tmpfs_open(struct vnode *file_node, struct file **target);
int tmpfs_close(struct file *file);

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

struct file_operations tmpfs_file_operations = {tmpfs_write,tmpfs_read,tmpfs_open,tmpfs_close};
struct vnode_operations tmpfs_vnode_operations = {tmpfs_lookup,tmpfs_create,tmpfs_mkdir};


int tmpfs_write(struct file *file, const void *buf, size_t len){
    tmpfs_node* internal = file -> vnode -> internal;
    for(int i = 0; i < len; i++){
        (internal -> data)[file -> f_pos + i] = ((char*)buf)[i];
    }
    file -> f_pos += len;
    if(internal -> size < file -> f_pos) {
        internal -> size = file -> f_pos;
	}
    return len;
}

int tmpfs_read(struct file *file, void *buf, size_t len){
    tmpfs_node* internal = file -> vnode -> internal;
    if(file -> f_pos + len > internal -> size) {
        len = internal -> size - file -> f_pos;
	} // read only to end
    for(int i = 0; i < len; i ++){
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
    my_free(file);
    return 0;
}

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){
	// Currently only below dir_node
    int idx = 0;
    tmpfs_node* internal = dir_node -> internal;
    while(internal -> entry[idx]){
        tmpfs_node* entry_node = internal -> entry[idx] -> internal;
        if(same(entry_node -> name, component_name)){
            *target = internal -> entry[idx];
            return 0;
        }
        idx++;
    }
    return -1;
}


vnode *tmpfs_create_vnode(int type){
    vnode* node = my_malloc(sizeof(vnode));
	memset(node, 0, sizeof(vnode));
    node -> f_ops = &tmpfs_file_operations;
    node -> v_ops = &tmpfs_vnode_operations;
    
	tmpfs_node *tnode = my_malloc(sizeof(tmpfs_node));
	memset(tnode, 0, sizeof(tmpfs_node));
    tnode -> type = type;
    tnode -> size = 0;
    node -> internal = tnode;
    return node;
}

int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
    //find an empty entry and initialize a vnode for it and set the name.
    tmpfs_node* internal = dir_node -> internal;
    if(internal -> type != 1) {
        uart_printf("not a dir\r\n");
        return -1;
    }

    int idx = -1;
    for(int i = 0; i < MAX_DIR_SIZE; i++){
        if(internal -> entry[i] == 0){
            idx = i;
            break;
        }
        struct tmpfs_node* infile = internal -> entry[i] -> internal;
        if(same(infile -> name, component_name)){
            uart_printf("file exisited\r\n");
            return -1;
        }
    }

    if(idx == -1){
        uart_printf ("dir full\r\n");
    }

    vnode* node = tmpfs_create_vnode(2);
	int t = strlen(component_name);
    strcpy(component_name, ((tmpfs_node*)node -> internal) -> name, t);
    internal -> entry[idx] = node;
    *target = node;
    return 0;
}

int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    //same as create
    struct tmpfs_node * internal = dir_node -> internal;
    if(internal -> type != 1) {
        uart_printf("not a dir\r\n");
        return -1;
    }

    int idx = -1;
    for(int i = 0; i < MAX_DIR_SIZE; i ++){
        if(internal -> entry[i] == 0){
            idx = i;
            break;
        }
        struct tmpfs_node* infile = internal -> entry[i] -> internal;
        if(same(infile -> name, component_name)){
            uart_printf("file exisited\r\n");
            return -1;
        }
    }

    if(idx == -1){
        uart_printf ("dir full\r\n");
    }

    vnode* node = tmpfs_create_vnode(1);
    tmpfs_node* inode = node -> internal;
    strcpy(component_name, inode -> name, strlen(component_name));
    internal -> entry[idx] = node;
    *target = node;
    return 0;
}

int tmpfs_mount(struct filesystem *fs, struct mount *mt){
    mt -> fs = fs;
    mt -> root = tmpfs_create_vnode(1);
    return 0;
}

#endif
