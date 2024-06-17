#ifndef UARTFS_H
#define UARTFS_H

#include "vfs.h"

typedef struct uart_dev_node{
    char name[MAX_PATH_SIZE];
    int type; // directory, mount, file
} uart_dev_node;

int uart_dev_write(struct file *file, const char *buf, size_t len);
int uart_dev_read(struct file *file, char *buf, size_t len);
int uart_dev_open(struct vnode *file_node, struct file **target);
int uart_dev_close(struct file *file);

int uart_dev_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int uart_dev_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int uart_dev_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

int uart_dev_mount(filesystem*, mount*);


#endif
