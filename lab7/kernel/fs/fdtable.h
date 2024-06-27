#ifndef _FDTABLE_H_
#define _FDTABLE_H_

# define MAX_FD 32

#include <lib/stddef.h>

struct file
{
    struct vnode *vnode;
    size_t f_pos; // RW position of this file handle
    struct file_operations *f_ops;
    int flags;
};

struct fdtable {
  int count;
  struct file* fds[MAX_FD];
};

#endif
