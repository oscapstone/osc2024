#include "fs_cpio.h"
#include "initrd.h"
#include "c_utils.h"
#include "mini_uart.h"

vnode cpio_root_node;
vnode mount_old_node;
int cpio_mounted;

// define struct
filesystem static_cpiofs = {
    .name = "cpiofs",
    .mount = cpiofs_mount
    // don't need alloc vnode because this fs is read-only
};

struct vnode_operations cpiofs_v_ops = {
    .lookup = cpiofs_lookup,
    .create = cpiofs_create,
    .mkdir = cpiofs_mkdir,
    .isdir = cpiofs_isdir,
    .getname = cpiofs_getname,
    .getsize = cpiofs_getsize,
};

struct file_operations cpiofs_f_ops = {
    .write = cpiofs_write,
    .read = cpiofs_read,
    .open = cpiofs_open,
    .close = cpiofs_close,
    .lseek64 = cpiofs_lseek64,
    .ioctl = cpiofs_ioctl,
};


// methods
int cpiofs_mount(filesystem *fs, mount *mnt) {
    vnode *backup_node, *cur_node;
    cpiofs_internal *internal;
    const char* name;

    if(cpio_mounted) {
        uart_send_string("cpiofs_mount: already mounted\n");
        return -1;
    }

    cur_node = mnt->root;

    cur_node -> v_ops -> getname(cur_node, &name);

    internal = cpio_root_node.internal;

    internal -> name = name;

    mount_old_node.mount = cur_node -> mount;
    mount_old_node.v_ops = cur_node -> v_ops;
    mount_old_node.f_ops = cur_node -> f_ops;
    mount_old_node.internal = cur_node -> internal;
    mount_old_node.parent = cur_node -> parent;

    cur_node -> mount = mnt;
    cur_node -> v_ops = cpio_root_node.v_ops;
    cur_node -> f_ops = cpio_root_node.f_ops;
    cur_node -> internal = internal;

    cpio_mounted = 1;

    return 0;
}

int cpiofs_lookup(vnode *dir_node, vnode **target, const char *component_name) {
    cpiofs_internal *internal, *entry;

    internal = dir_node -> internal;

    if(internal -> type != CPIOFS_TYPE_DIR) {
        uart_send_string("cpiofs_lookup: not a directory\n");
        return -1;
    }

    list_for_each_entry(entry, &internal -> dir.list, list) {
        uart_send_string("cpiofs_lookup: entry -> name: ");
        uart_send_string(entry -> name);
        uart_send_string("\n");
        if(strcmp(entry -> name, component_name) == 0) {
            break;
        }
    }

    if(&entry -> list == &internal -> dir.list) {
        uart_send_string("cpiofs_lookup: file not found\n");
        return -1;
    }
    char* name;
    uart_send_string("entry -> name: ");
    uart_send_string(entry -> name);
    uart_send_string("\n");
    uart_send_string("entry -> internal -> name: ");
    entry -> node -> v_ops -> getname(entry -> node, &name);
    uart_send_string(name);
    uart_send_string("\n");

    *target = entry -> node;

    return 0;
}

int cpiofs_create(vnode *dir_node, vnode **target, const char *component_name) {
    return -1;
}

int cpiofs_mkdir(vnode *dir_node, vnode **target, const char *component_name) {
    return -1;
}

int cpiofs_isdir(vnode *dir_node) {
    cpiofs_internal *internal;

    internal = dir_node -> internal;

    if (internal -> type != CPIOFS_TYPE_DIR) {
        return 0;
    }

    return 1;
}

int cpiofs_getname(vnode *dir_node, const char **name) {
    cpiofs_internal *internal = dir_node -> internal;
    *name = internal -> name;

    return 0;
}

int cpiofs_getsize(vnode *dir_node) {
    cpiofs_internal *internal = dir_node -> internal;
    if (internal -> type == CPIOFS_TYPE_DIR) {
        uart_send_string("cpiofs_getsize: is a directory\n");
        return -1;
    }

    return internal -> file.size;
}

int cpiofs_open(vnode *file_node, file *target) {
    target -> f_ops = file_node -> f_ops;
    target -> vnode = file_node;
    target -> f_pos = 0;

    return 0;
}

int cpiofs_close(file *target) {
    target -> f_ops = NULL;
    target -> vnode = NULL;
    target -> f_pos = 0;

    return 0;
}

int cpiofs_write(file *target, const void *buf, size_t len) {
    return -1;
}

int cpiofs_read(file *target, void *buf, size_t len) {
    cpiofs_internal* internal =  target -> vnode -> internal;
    uart_send_string("cpiofs_read: internal -> name: ");
    uart_send_string(internal -> name);
    uart_send_string("\n");
    uart_send_string("cpiofs_read: internal -> type: ");
    uart_hex(internal -> type);
    uart_send_string("\n");
    if(internal -> type != CPIOFS_TYPE_FILE) {
        uart_send_string("cpiofs_read: not a file\n");
        return -1;
    } 

    if(len > internal -> file.size - target -> f_pos) {
        len = internal -> file.size - target -> f_pos;
    }

    if(!len) {
        return 0;
    }
    memcpy(buf, internal -> file.data + target -> f_pos, len);
    target -> f_pos += len;

    return len;
}

long cpiofs_lseek64(file *target, long offset, int whence) {
    int filesize;
    int base;

    filesize = target -> vnode -> v_ops -> getsize(target -> vnode);

    if (filesize < 0) {
        return -1;
    }

    switch(whence) {
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
        return -1;
    }

    target -> f_pos = base + offset;

    return 0;
}

int cpiofs_ioctl(struct file *file, uint64_t request, va_list args) {
    return -1;
}

uint32_t cpio_read_8hex(const char *s) {
	int r = 0;
    int n = 8;
	while (n-- > 0) {
		r = r << 4;
		if (*s >= 'A')
			r += *s++ - 'A' + 10;
		else if (*s >= '0')
			r += *s++ - '0';
	}
	return r;
}

vnode *cpio_get_vnode_from_path(vnode *dir_node, const char **name) {
    vnode* result;
    const char* start;
    const char* end;
    char buf[256];

    start = end = *name;

    if(*start == '/') {
        result = &cpio_root_node;
    } else {
        result = dir_node;
    }

    while(1) {
        if(!strncmp("./", start, 2)) {
            start += 2;
            end = start;
            continue;
        } else if(!strncmp("../", start, 3)) {
            if(result -> parent) {
                result = result -> parent;
            }
            start += 3;
            end = start;
            continue;
        }

        while(*end != '\0' && *end != '/') {
            end++;
        }

        if(*end == '/') {
            int ret;

            if(start == end) {
                end ++;
                start = end;
                continue;
            }

            memcpy(buf, start, end - start);
            buf[end - start] = '\0';

            ret = result -> v_ops -> lookup(result, &result, buf);
            uart_send_string("cpio_get_vnode_from_path lookup ret: ");
            uart_hex(ret);
            uart_send_string("\n");
            if(ret < 0) {
                return 0;
            }

            end ++;
            start = end;
        } else {
            break;
        }
    }

    *name = *start ? start : 0;

    return result;
}

void cpio_init_mkdir(const char *pathname) {
    char* curname;
    vnode *dir_node;
    vnode *new_dir_node;
    cpiofs_internal *internal;
    cpiofs_internal *new_internal;
    // use kmalloc to allocate memory
    curname = (char *)kmalloc(strlen(pathname) + 1);
    strcpy(curname, pathname);
    dir_node = cpio_get_vnode_from_path(&cpio_root_node, &curname);

    if(!dir_node) {
        uart_send_string("cpio_init_mkdir: directory not found\n");
        return;
    }

    if(!curname) {
        uart_send_string("cpio_init_mkdir: invalid pathname\n");
        return;
    }

    internal = dir_node -> internal;

    uart_send_string("cpio_init_mkdir: internal -> name: ");
    if(!internal -> name) {
        uart_send_string("/");
    } else {
        uart_send_string(internal -> name);
    }
    uart_send_string("\n");

    if(internal -> type != CPIOFS_TYPE_DIR) {
        uart_send_string("cpio_init_mkdir: not a directory\n");
        return;
    }

    new_internal = (cpiofs_internal *)kmalloc(sizeof(cpiofs_internal));
    new_dir_node = (vnode *)kmalloc(sizeof(vnode));
    uart_send_string("cpio_init_mkdir: new_internal -> name: ");
    uart_send_string(curname);
    uart_send_string("\n");
    uart_send_string("new_dir_node addr: ");
    uart_hex((unsigned long)new_dir_node);
    uart_send_string("\n");
    new_internal -> name = curname;
    new_internal -> type = CPIOFS_TYPE_DIR;
    INIT_LIST_HEAD(&new_internal -> dir.list);
    new_internal -> node = new_dir_node;
    list_add_tail(&new_internal -> list, &internal -> dir.list);

    new_dir_node -> mount = 0;
    new_dir_node -> v_ops = &cpiofs_v_ops;
    new_dir_node -> f_ops = &cpiofs_f_ops;
    new_dir_node -> internal = new_internal;
    new_dir_node -> parent = dir_node;

    uart_send_string("[success] cpio_init_mkdir: create directory ");
    uart_send_string(curname);
    uart_send_string("\n");

    return;
}

void cpio_init_create(const char* pathname, const char *data, uint64_t size) {
    char *curname;
    vnode *dir_node;
    vnode *new_file_node;
    cpiofs_internal *internal, *new_internal;

    curname = (char *)kmalloc(strlen(pathname) + 1);
    strcpy(curname, pathname);
    dir_node = cpio_get_vnode_from_path(&cpio_root_node, &curname);

    if(!dir_node){
        uart_send_string("cpio_init_create: directory not found\n");
        return;
    }
    if(!curname) {
        uart_send_string("cpio_init_create: invalid pathname\n");
        return;
    }

    char *dir_name = (char*)kmalloc(256);
    dir_node -> v_ops -> getname(dir_node, &dir_name);


    internal = dir_node -> internal;

    if(internal -> type != CPIOFS_TYPE_DIR) {
        uart_send_string("cpio_init_create: not a directory\n");
        return;
    }

    uart_send_string("cpio_init_create: internal -> name: ");
    if(!internal -> name) {
        uart_send_string("/");
    } else {
        uart_send_string(internal -> name);
    }
    uart_send_string("\n");


    new_internal = (cpiofs_internal *)kmalloc(sizeof(cpiofs_internal));
    new_file_node = (vnode *)kmalloc(sizeof(vnode));

    new_internal -> name = curname;
    new_internal -> type = CPIOFS_TYPE_FILE;
    new_internal -> file.data = data;
    new_internal -> file.size = size;
    new_internal -> node = new_file_node;
    list_add_tail(&new_internal -> list, &internal -> dir.list);

    new_file_node -> mount = 0;
    new_file_node -> v_ops = &cpiofs_v_ops;
    new_file_node -> f_ops = &cpiofs_f_ops;
    new_file_node -> internal = new_internal;
    new_file_node -> parent = dir_node;

    uart_send_string("[success] cpio_init_create: create file ");
    uart_send_string(curname);
    uart_send_string("\n");

    return;
} 

filesystem *cpiofs_init(void) {
    cpiofs_internal *internal;

    internal = (cpiofs_internal *)kmalloc(sizeof(cpiofs_internal));

    internal -> name = 0;
    internal -> type = CPIOFS_TYPE_DIR;
    INIT_LIST_HEAD(&internal -> dir.list);
    internal -> node = &cpio_root_node;
    INIT_LIST_HEAD(&internal -> list);

    cpio_root_node.mount = 0;
    cpio_root_node.v_ops = &cpiofs_v_ops;
    cpio_root_node.f_ops = &cpiofs_f_ops;
    cpio_root_node.internal = internal;
    cpio_root_node.parent = 0;

    cpio_t* header = ramfs_base;

    while(header) {
        char *component_name, *data;
        uint32_t namesize, filesize, aligned_namesize, aligned_filesize, type;
        
        if(strncmp(header -> c_magic, "070701", sizeof(header->c_magic)) != 0) {
            uart_send_string("cpiofs_init: invalid cpio format\n");
            break;
        }

        namesize = cpio_read_8hex(header -> c_namesize);
        filesize = cpio_read_8hex(header -> c_filesize);
        type = cpio_read_8hex(header -> c_mode) & CPIO_TYPE_MASK;

        component_name = ((char *)header) + sizeof(cpio_t);

        aligned_namesize = align4(namesize);
        aligned_filesize = align4(filesize);
        unsigned int offset = namesize + sizeof(cpio_t);
        offset = offset % 4 == 0 ? offset : (offset + 4 - offset % 4); // padding


        // uart_send_string("file name: ");
        // uart_send_string(component_name);
        // uart_send_string("\n");

        if (strncmp(component_name, "TRAILER!!!", sizeof("TRAILER!!!")) == 0) {
            break;
        }

        data = (char*)header + offset;
        if(filesize == 0) {
            header = (cpio_t *)data;
        } else {
            offset = filesize;
            header = (cpio_t *)(data + (offset % 4 == 0 ? offset : (offset + 4 - offset % 4)));
        }

        if(type == CPIO_TYPE_FILE) {
            uart_send_string("cpiofs_init: create file ");
            uart_send_string(component_name);
            uart_send_string("\n");
            cpio_init_create(component_name, data, filesize);
        } else if(type == CPIO_TYPE_DIR) {
            uart_send_string("cpiofs_init: mkdir ");
            uart_send_string(component_name);
            uart_send_string("\n");
            cpio_init_mkdir(component_name);
        }
    }

    return &static_cpiofs;
}
