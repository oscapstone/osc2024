#ifndef __TMPFS_H_
#define __TMPFS_H_

#include "vfs.h"

#define COMPONENT_SIZE 16


fs_ptr      tmpfs_create();

extern struct file_operations   tmpfs_f_ops;
extern struct vnode_operations  tmpfs_v_ops;

#endif
