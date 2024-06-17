#ifndef FRAMEBUFFER_H 
#define FRAMEBUFFER_H 

#include "vfs.h"

typedef struct framebuffer_node{
    char name[MAX_PATH_SIZE];
    int type; // directory, mount, file
	unsigned int width, height, pitch, isrgb;
	char* lfb;
} framebuffer_node;

int framebuffer_write(struct file *file, const char *buf, size_t len);
int framebuffer_read(struct file *file, char *buf, size_t len);
int framebuffer_open(struct vnode *file_node, struct file **target);
int framebuffer_close(struct file *file);

int framebuffer_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int framebuffer_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int framebuffer_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

int framebuffer_mount(filesystem*, mount*);


#endif
