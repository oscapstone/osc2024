#include "fs_tmpfs.h"
#include "alloc.h"
#include "string.h"


// define struct
filesystem static_tmpfs = {
    .name = "tmpfs",
    .mount = tmpfs_mount,
    .alloc_vnode = tmpfs_alloc_vnode,
};

struct vnode_operations tmpfs_v_ops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir,
    .isdir = tmpfs_isdir,
    .getname = tmpfs_getname,
    .getsize = tmpfs_getsize,
};

struct file_operations tmpfs_f_ops = {
    .write = tmpfs_write,
    .read = tmpfs_read,
    .open = tmpfs_open,
    .close = tmpfs_close,
    .lseek64 = tmpfs_lseek64,
};

filesystem *tmpfs_init(void) {
    return &static_tmpfs;
}

int tmpfs_mount(filesystem *fs, mount *mnt) {
    vnode *backup_node, *cur_node;
    tmpfs_internal *internal;
    tmpfs_dir *dir;
    const char* name;

    cur_node = mnt->root;

    cur_node -> v_ops -> getname(cur_node, &name);

    if(strlen(name) >= TMPFS_NAME_MAXLEN) {
        uart_send_string("tmpfs_mount: name too long\n");
        return -1;
    }

    backup_node = (vnode *)kmalloc(sizeof(vnode));
    internal = (tmpfs_internal *)kmalloc(sizeof(tmpfs_internal));
    dir = (tmpfs_dir *)kmalloc(sizeof(tmpfs_dir));

    dir -> size = 0;

    backup_node -> mount = mnt;
    backup_node -> v_ops = cur_node -> v_ops;
    backup_node -> f_ops = cur_node -> f_ops;
    backup_node -> internal = cur_node -> internal;

    strcpy(internal -> name, name);
    internal -> type = TMPFS_TYPE_DIR;
    internal -> dir = dir;
    internal -> old_node = backup_node;

    cur_node -> mount = mnt;
    cur_node -> v_ops = &tmpfs_v_ops;
    cur_node -> f_ops = &tmpfs_f_ops;
    cur_node -> internal = internal;

    return 0;
}

int tmpfs_alloc_vnode(filesystem *fs, vnode **target) {
    vnode *node;
    tmpfs_internal *internal;
    tmpfs_dir *dir;

    node = (vnode *)kmalloc(sizeof(vnode));
    internal = (tmpfs_internal *)kmalloc(sizeof(tmpfs_internal));
    dir = (tmpfs_dir *)kmalloc(sizeof(tmpfs_dir));

    dir -> size = 0;

    internal -> name[0] = '\0';
    internal -> type = TMPFS_TYPE_DIR;
    internal -> dir = dir;
    internal -> old_node = 0;

    node -> mount = 0;
    node -> v_ops = &tmpfs_v_ops;
    node -> f_ops = &tmpfs_f_ops;
    node -> internal = internal;

    *target = node;

    return 0;
}


// vnode operations
int tmpfs_lookup(vnode *dir_node, vnode **target, const char *component_name) {
    // uart_send_string("tmpfs_lookup\n");
    // uart_send_string(component_name);
    // uart_send_string("\n");
    tmpfs_internal *internal;
    tmpfs_dir *dir;
    vnode *node;
    int i;

    internal = (tmpfs_internal *)dir_node -> internal;

    if(internal -> type != TMPFS_TYPE_DIR) {
        uart_send_string("tmpfs_lookup: not a dir\n");
        return -1;
    }
    // uart_send_string("tmpfs_lookup: is a dir size: ");
    dir = internal -> dir;
    
    
    // uart_hex(dir -> size);
    // uart_send_string("\n");

    for(i = 0; i < dir -> size; i++) {
        // uart_send_string("tmpfs_lookup i: ");
        // uart_hex(i);
        // uart_send_string("\n");
        const char *name;
        int ret;
        node = dir -> files[i];
        ret = node -> v_ops -> getname(node, &name);
        if(ret < 0){
            continue;
        }
        if(strcmp(name, component_name) == 0) {
            break;
        }
    }
    

    if(i >= dir -> size) {
        uart_send_string("tmpfs_lookup: not found\n");
        return -1;
    }

    *target = dir -> files[i];

    return 0;
}

// create a new file under dir_node
// return 0 on success, -1 on error
int tmpfs_create(vnode *dir_node, vnode **target, const char *component_name) {
    tmpfs_internal *internal, *new_internal;
    tmpfs_file *file;
    tmpfs_dir *dir;
    vnode *node;
    int ret;

    if(strlen(component_name) >= TMPFS_NAME_MAXLEN) {
        uart_send_string("tmpfs_create: name too long\n");
        return -1;
    }

    internal = (tmpfs_internal *)dir_node -> internal;
    if(internal -> type != TMPFS_TYPE_DIR) {
        uart_send_string("tmpfs_create: not a dir\n");
        return -1;
    }

    dir = internal -> dir;

    if(dir -> size >= TMPFS_DIR_MAXSIZE) {
        uart_send_string("tmpfs_create: dir full\n");
        return -1;
    }

    ret = tmpfs_lookup(dir_node, &node, component_name);

    if(!ret){
        uart_send_string("tmpfs_create: file exists\n");
        return -1;
    }

    node = (vnode *)kmalloc(sizeof(vnode));
    new_internal = (tmpfs_internal *)kmalloc(sizeof(tmpfs_internal));
    file = (tmpfs_file *)kmalloc(sizeof(tmpfs_file));

    file -> data = kmalloc(TMPFS_FILE_MAXSIZE);
    file -> size = 0;
    file -> capacity = TMPFS_FILE_MAXSIZE;

    strcpy(new_internal -> name, component_name);
    new_internal -> type = TMPFS_TYPE_FILE;
    new_internal -> file = file;
    new_internal -> old_node = 0;

    node -> mount = dir_node -> mount;
    node -> v_ops = &tmpfs_v_ops;
    node -> f_ops = &tmpfs_f_ops;
    node -> parent = dir_node;
    node -> internal = new_internal;

    dir -> files[dir -> size] = node;
    dir -> size ++;

    *target = node;

    return 0;
}

int tmpfs_mkdir(vnode *dir_node, vnode **target, const char *component_name) {
    tmpfs_internal *internal, *new_internal;
    tmpfs_dir *dir, *new_dir;
    vnode *node;
    int ret;

    if(strlen(component_name) >= TMPFS_NAME_MAXLEN) {
        uart_send_string("tmpfs_create: name too long\n");
        return -1;
    }

    internal = (tmpfs_internal *)dir_node -> internal;
    if(internal -> type != TMPFS_TYPE_DIR) {
        uart_send_string("tmpfs_create: not a dir\n");
        return -1;
    }

    dir = internal -> dir;

    if(dir -> size >= TMPFS_DIR_MAXSIZE) {
        uart_send_string("tmpfs_create: dir full\n");
        return -1;
    }

    ret = tmpfs_lookup(dir_node, &node, component_name);

    if(!ret){
        uart_send_string("tmpfs_create: file exists\n");
        return -1;
    }

    node = (vnode *)kmalloc(sizeof(vnode));
    new_internal = (tmpfs_internal *)kmalloc(sizeof(tmpfs_internal));
    new_dir = (tmpfs_dir *)kmalloc(sizeof(tmpfs_dir));

    new_dir -> size = 0;

    strcpy(new_internal -> name, component_name);
    new_internal -> type = TMPFS_TYPE_DIR;
    new_internal -> dir = new_dir;
    new_internal -> old_node = 0;

    node -> mount = dir_node -> mount;
    node -> v_ops = &tmpfs_v_ops;
    node -> f_ops = &tmpfs_f_ops;
    node -> parent = dir_node;
    node -> internal = new_internal;

    dir -> files[dir -> size] = node;
    dir -> size ++;

    *target = node;

    return 0;
}

int tmpfs_isdir(vnode *dir_node) {
    tmpfs_internal *internal =  (tmpfs_internal *)dir_node -> internal;
    if (internal -> type == TMPFS_TYPE_DIR) {
        return 1;
    }

    return 0;
}

int tmpfs_getname(vnode *dir_node, const char **name) {
    tmpfs_internal *internal = internal = (tmpfs_internal *)dir_node -> internal;
    *name = internal -> name;
    return 0;
}

int tmpfs_getsize(vnode *dir_node) {
    tmpfs_internal *internal = (tmpfs_internal *)dir_node -> internal;
    return internal -> type == TMPFS_TYPE_FILE ? internal -> file -> size : -1;
}

// file operations
int tmpfs_open(vnode *file_node, file *target) {
    if(((tmpfs_internal*)file_node -> internal) -> type != TMPFS_TYPE_FILE) {
        uart_send_string("tmpfs_open: not a file\n");
        return -1;
    }
    target -> vnode = file_node;
    target -> f_pos = 0;
    target -> f_ops = file_node -> f_ops;

    return 0;
}

int tmpfs_close(file *target) {
    target -> vnode = 0;
    target -> f_ops = 0;
    target -> f_pos = 0;

    return 0;
}

int tmpfs_write(file *target, const void *buf, size_t len) {
    tmpfs_internal *internal = (tmpfs_internal *)target -> vnode -> internal;
    size_t i;

    if(internal -> type != TMPFS_TYPE_FILE) {
        uart_send_string("tmpfs_write: not a file\n");
        return -1;
    }

    tmpfs_file *file = internal -> file;

    if(len > file -> capacity - file -> size) {
        len = file -> capacity - file -> size;
    }

    if(!len) {
        uart_send_string("tmpfs_write: no space left\n");
        return 0;
    }

    memcpy(&file->data[target->f_pos], buf, len);

    target -> f_pos += len;

    if(target -> f_pos > file -> size) {
        file -> size = target -> f_pos;
    }

    return len;
}

int tmpfs_read(file *target, void *buf, size_t len) {
    tmpfs_internal *internal = (tmpfs_internal *)target -> vnode -> internal;

    if(internal -> type != TMPFS_TYPE_FILE) {
        uart_send_string("tmpfs_read: not a file\n");
        return -1;
    }

    tmpfs_file *file = internal -> file;

    if(len > file -> size - target -> f_pos) {
        len = file -> size - target -> f_pos;
    }

    memcpy(buf, &file -> data[target -> f_pos], len);

    target -> f_pos += len;

    return len;
}


long tmpfs_lseek64(file *target, long offset, int whence) {
    int filesize;
    int base;

    filesize = target -> vnode -> v_ops -> getsize(target -> vnode);

    if(filesize < 0) {
        return -1;
    }

    switch (whence) {
        case SEEK_SET:
            base = 0;
            break;
        case SEEK_CUR:
            base = target -> f_pos;
            break;
        case SEEK_END:
            base = filesize;
            break;
        default:
            return -1;
    }

    if(base + offset > filesize) {
        uart_send_string("tmpfs_lseek64: invalid offset\n");
        return -1;
    }

    target -> f_pos = base + offset;

    return 0;
}
