#include "fs_uartfs.h"
#include "fs_tmpfs.h"

filesystem *uartfs_init(void) {
    return &static_uartfs;
};

filesystem static_uartfs = {
    .name = "uartfs",
    .mount = uartfs_mount,
};

vnode_operations uartfs_v_ops = {
    .lookup = uartfs_lookup,
    .create = uartfs_create,
    .mkdir = uartfs_mkdir,
    .isdir = uartfs_isdir,
    .getname = uartfs_getname,
    .getsize = uartfs_getsize,
};

file_operations uartfs_f_ops = {
    .write = uartfs_write,
    .read = uartfs_read,
    .open = uartfs_open,
    .close = uartfs_close,
    .lseek64 = uartfs_lseek64,
    .ioctl = uartfs_ioctl,
};

int uartfs_mount(filesystem *fs, mount *mnt) {
    vnode *cur_node;
    uartfs_internal *internal;
    const char* name;
    internal = (uartfs_internal *)kmalloc(sizeof(uartfs_internal));

    cur_node = mnt -> root;
    cur_node -> v_ops -> getname(cur_node, &name);

    internal -> name = name;
    internal -> oldnode.mount = cur_node -> mount;
    internal -> oldnode.v_ops = cur_node -> v_ops;
    internal -> oldnode.f_ops = cur_node -> f_ops;
    internal -> oldnode.parent = cur_node -> parent;
    internal -> oldnode.internal = cur_node -> internal;

    cur_node -> mount = mnt;
    cur_node -> v_ops = &uartfs_v_ops;
    cur_node -> f_ops = &uartfs_f_ops;
    cur_node -> internal = internal;

    return 0;
}


int uartfs_lookup(vnode *dir_node, vnode **target, const char *component_name) {
    return -1;
}

int uartfs_create(vnode *dir_node, vnode **target, const char *component_name) {
    return -1;
}

int uartfs_mkdir(vnode *dir_node, vnode **target, const char *component_name) {
    return -1;
}

int uartfs_isdir(vnode *dir_node) {
    return 0;
}

int uartfs_getname(vnode *dir_node, const char **name) {
    uartfs_internal *internal = (uartfs_internal *)dir_node -> internal;

    *name = internal -> name;
    return 0;
}

int uartfs_getsize(vnode *dir_node) {
    return -1;
}


int uartfs_open(vnode *file_node, file *target) {
    target -> vnode = file_node;
    target -> f_pos = 0;
    target -> f_ops = file_node -> f_ops;

    return 0;
}

int uartfs_close(file *target) {
    target -> vnode = NULL;
    target -> f_pos = 0;
    target -> f_ops = NULL;

    return 0;
}

int uartfs_write(file *target, const void *buf, size_t len) {
    uart_sendn(buf, len);

    return len;
}

int uartfs_read(file *target, void *buf, size_t len) {
    uart_recvn(buf, len);

    return len;
}

long uartfs_lseek64(file *target, long offset, int whence) {
    return -1;
}

int uartfs_ioctl(struct file *file, uint64_t request, va_list args) {
    return -1;
}