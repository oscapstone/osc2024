#include "fs_vfs.h"
#include "thread.h"
#include "string.h"
#include "alloc.h"
#include "mini_uart.h"
#include "reboot.h"
#include "thread.h"

mount* rootfs;

struct list_head fs_lists;

vnode* get_vnode_from_path(vnode* dir_node, const char **pathname) {
    uart_send_string("get_vnode_from_path\n");
    vnode* result;
    const char *start;
    const char *end;
    char buf[256];

    start = end = *pathname;
    
    if(*start == '/') {
        result = rootfs->root;
    } else {
        result = dir_node;
    }
    uart_send_string("get_vnode_from_path: ");
    uart_send_string(*pathname);
    uart_send_string("\n");


    // find the vnode from the path
    while(1) {
        // handle ./ and ../
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
                // handle the case like /usr//bin
                end ++;
                start = end;
                continue;
            }

            memcpy(buf, start, end - start);
            buf[end - start] = '\0';
            uart_send_string(buf);
            uart_send_string("\n");

            ret = result -> v_ops -> lookup(result, &result, buf);
            // failed
            if(ret < 0) {
                return NULL;
            }

            end ++;
            start = end;
        } else {
            // reach the end of the path
            break;
        }
    }

    *pathname = *start ? start : NULL;
    
    return result;
}

filesystem* find_fs(const char *fs_name) {
    struct filesystem *fs;

    list_for_each_entry(fs, &fs_lists, fs_list) {
        if(!strcmp(fs -> name, fs_name)) {
            return fs;
        }
    }

    return NULL;
}

void vfs_init() {
    INIT_LIST_HEAD(&fs_lists);
}

void vfs_init_rootfs(filesystem *fs) {
    // original code: vnode* new_vnode;
    vnode *new_vnode = (vnode*)kmalloc(sizeof(vnode));
    int ret;

    ret = fs -> alloc_vnode(fs, &new_vnode);
    if(ret < 0) {
        uart_send_string("vfs_init_rootfs: failed to alloc vnode\n");
        reset(1);
        return;
    }

    rootfs = (mount*)kmalloc(sizeof(mount));

    rootfs -> fs = fs;
    rootfs -> root = new_vnode;
    new_vnode -> mount = rootfs;
    new_vnode -> parent = NULL;
}

int register_filesystem(filesystem *fs) {
    if(find_fs(fs -> name)) {
        uart_send_string("register_filesystem: filesystem already registered\n");
        return -1;
    }

    list_add_tail(&fs -> fs_list, &fs_lists);

    return 0;
}

int vfs_open(const char* pathname, int flags, file* target) {
    const char* cur_name = pathname;
    vnode* dir_node;
    vnode* file_node;
    int ret;
    
    if(is_init_thread){
        dir_node = get_vnode_from_path(get_current_thread() -> working_dir, &cur_name);
    } else {
        dir_node = get_vnode_from_path(rootfs -> root, &cur_name);
    }

    if(!dir_node) {
        uart_send_string("vfs_open: failed to get dir node\n");
        return -1;
    }

    if(!cur_name) {
        uart_send_string("vfs_open: invalid pathname\n");
        return -1;
    }
    char* name;
    dir_node -> v_ops -> getname(dir_node, &name);

    ret = dir_node -> v_ops -> lookup(dir_node, &file_node, cur_name);


    if(flags & O_CREAT) {
        if(ret == 0){
            uart_send_string("vfs_open: file already exists\n");
            return -1;
        }

        ret = dir_node -> v_ops -> create(dir_node, &file_node, cur_name);
        uart_send_string("vfs_open: create success\n");

    }

    if (ret < 0) {
        uart_send_string("vfs_open: failed to lookup/create file\n");
        return ret;
    }

    if(!file_node) {
        uart_send_string("vfs_open: failed to get file node\n");
        return -1;
    }
    char* name2;
    file_node -> v_ops -> getname(file_node, &name2);
    uart_send_string("vfs_open: ");
    uart_send_string(name2);
    uart_send_string("\n");

    ret = file_node -> f_ops -> open(file_node, target);
    if(ret < 0){
        uart_send_string("vfs_open: failed to open file\n");
        return ret;
    }

    target -> flags = 0;

    return 0;
}

int vfs_close(file *f) {
    return f -> f_ops -> close(f);
}

int vfs_write(file *f, const void *buf, size_t len) {
    return f -> f_ops -> write(f, buf, len);
}

int vfs_read(file *f, void *buf, size_t len) {
    return f -> f_ops -> read(f, buf, len);
}

int vfs_mkdir(const char* pathname) {
    char* cur_name = (char*)kmalloc(strlen(pathname) + 1);
    strcpy(cur_name, pathname);
    vnode* dir_node;
    vnode* new_node;
    int ret;
    uart_send_string("vfs_mkdir: ");
    uart_send_string(cur_name);
    uart_send_string("\n");
    
    if(is_init_thread){
        dir_node = get_vnode_from_path(get_current_thread() -> working_dir, &cur_name);
    } else {
        dir_node = get_vnode_from_path(rootfs -> root, &cur_name);
    }

    if(!dir_node) {
        uart_send_string("vfs_mkdir: failed to get dir node\n");
        return -1;
    }

    if(!cur_name) {
        uart_send_string("vfs_mkdir: invalid pathname\n");
        return -1;
    }

    ret = dir_node -> v_ops -> mkdir(dir_node, &new_node, cur_name);

    if(ret < 0) {
        uart_send_string("vfs_mkdir: failed to mkdir\n");
        return ret;
    }

    uart_send_string("vfs_mkdir: success\n");

    return 0;
}

int vfs_mount(const char* target, const char* fs_name) {
    const char* cur_name = target;
    vnode* dir_node;
    filesystem* fs;
    mount* mnt;
    int ret;
    
    if(is_init_thread) {
        dir_node = get_vnode_from_path(get_current_thread() -> working_dir, &cur_name);
    } else {
        dir_node = get_vnode_from_path(rootfs -> root, &cur_name);
    }

    if(!dir_node) {
        uart_send_string("vfs_mount: failed to get dir node\n");
        return -1;
    }

    if(cur_name) {
        ret = dir_node -> v_ops -> lookup(dir_node, &dir_node, cur_name);

        if(ret < 0) {
            return ret;
        }
    }

    if(!dir_node -> v_ops -> isdir(dir_node)) {
        uart_send_string("vfs_mount: target is not a directory\n");
        return -1;
    }

    fs = find_fs(fs_name);

    if(!fs) {
        uart_send_string("vfs_mount: filesystem not found\n");
        return -1;
    }

    mnt = (mount*)kmalloc(sizeof(mount));

    mnt -> fs = fs;
    mnt -> root = dir_node;

    ret = fs -> mount(fs, mnt);

    if(ret < 0) {
        uart_send_string("vfs_mount: failed to mount\n");
        kfree(mnt);
        return ret;
    }
}

int vfs_lookup(const char *pathname, vnode **target) {
    char* cur_name = pathname;
    vnode* dir_node;
    vnode* file_node;
    int ret;
    uart_send_string("vfs_lookup: ");
    uart_send_string(cur_name);
    uart_send_string("\n");
    dir_node = get_vnode_from_path(get_current_thread() -> working_dir, &cur_name);
    uart_send_string("vfs_lookup: ");
    uart_send_string(cur_name);
    uart_send_string("\n");

    if(!dir_node) {
        uart_send_string("vfs_lookup: failed to get dir node\n");
        return -1;
    }

    if(!cur_name) {
        // found the directory node as target
        *target = dir_node;
        return 0;
    }

    ret = dir_node -> v_ops -> lookup(dir_node, &file_node, cur_name);

    if(ret < 0) {
        uart_send_string("vfs_lookup: failed to lookup file\n");
        return ret;
    }

    *target = file_node;

    return 0;
}

long vfs_lseek64(file *f, long offset, int whence) {
    return f -> f_ops -> lseek64(f, offset, whence);
}

int vfs_ioctl(file *f, uint64_t request, va_list args) {
    return f -> f_ops -> ioctl(f, request, args);
}

// wrapper's 
int open_wrapper(const char* pathname, int flags) {
    int i, ret;
    for(i = 0; i <= get_current_thread() -> max_fd; i++ ) {
        if(get_current_thread() -> fds[i].vnode == 0) {
            break;
        }
    }

    if(i > get_current_thread() -> max_fd) {
        if(get_current_thread() -> max_fd >= THREAD_MAX_FD) {
            uart_send_string("open_wrapper: too many files opened\n");
            return -1;
        }

        get_current_thread() -> max_fd ++;
        i = get_current_thread() -> max_fd;
    }

    // uart_send_string("open_wrapper: ");
    // uart_send_string(pathname);
    // uart_send_string("\n");
    // int tid = get_current_thread() -> tid;
    // uart_send_string("tid: ");
    // uart_hex(tid);
    // uart_send_string("\n");
    // uart_send_string("i: ");
    // uart_hex(i);
    // uart_send_string("\n");
    // uart_send_string("flags: ");
    // uart_hex(flags);
    // uart_send_string("\n");

    ret = vfs_open(pathname, flags, &get_current_thread() -> fds[i]);
    
    if(ret < 0) {
        uart_send_string("open_wrapper: failed to open file\n");
        get_current_thread() -> fds[i].vnode = 0;
        return -1;
    }

    return i;
}

int close_wrapper(int fd) {
    if(fd < 0 || fd > get_current_thread() -> max_fd) {
        uart_send_string("close_wrapper: invalid fd\n");
        return -1;
    }

    if(get_current_thread() -> fds[fd].vnode == 0) {
        uart_send_string("close_wrapper: file not opened\n");
        return -1;
    }

    int ret;
    ret = vfs_close(&get_current_thread() -> fds[fd]);

    if(ret < 0) return ret;

    return 0;
}

int write_wrapper(int fd, const void *buf, size_t len) {
    if(fd < 0 || fd > get_current_thread() -> max_fd) {
        uart_send_string("write_wrapper: invalid fd\n");
        return -1;
    }

    if(get_current_thread() -> fds[fd].vnode == 0) {
        uart_send_string("write_wrapper: file not opened\n");
        return -1;
    }

    int ret;
    ret = vfs_write(&get_current_thread() -> fds[fd], buf, len);

    return ret;
}

int read_wrapper(int fd, void *buf, size_t len) {
    if(fd < 0 || fd > get_current_thread() -> max_fd) {
        uart_send_string("read_wrapper: invalid fd\n");
        return -1;
    }

    if(get_current_thread() -> fds[fd].vnode == 0) {
        uart_send_string("read_wrapper: file not opened\n");
        return -1;
    }

    int ret;
    ret = vfs_read(&get_current_thread() -> fds[fd], buf, len);

    return ret;
}

int mkdir_wrapper(const char* pathname) {
    return vfs_mkdir(pathname);
}

int mount_wrapper(const char* target, const char* fs_name) {
    return vfs_mount(target, fs_name);
}

int chdir_wrapper(const char* path) {
    vnode* target;
    int ret;

    ret = vfs_lookup(path, &target);

    if(ret < 0) {
        uart_send_string("chdir_wrapper: failed to lookup\n");
        return -1;
    }

    if(!target -> v_ops -> isdir(target)) {
        uart_send_string("chdir_wrapper: not a directory\n");
        return -1;
    }

    get_current_thread() -> working_dir = target;

    return 0;
}

long lseek64_wrapper(int fd, long offset, int whence) {
    if(fd < 0 || fd > get_current_thread() -> max_fd) {
        uart_send_string("lseek64_wrapper: invalid fd\n");
        return -1;
    }

    if(get_current_thread() -> fds[fd].vnode == 0) {
        uart_send_string("lseek64_wrapper: file not opened\n");
        return -1;
    }

    int ret;
    ret = vfs_lseek64(&get_current_thread() -> fds[fd], offset, whence);

    return ret;
}

int ioctl_wrapper(int fd, uint64_t cmd, va_list args) {
    if(fd < 0 || fd > get_current_thread() -> max_fd) {
        uart_send_string("ioctl_wrapper: invalid fd\n");
        return -1;
    }

    if(get_current_thread() -> fds[fd].vnode == 0) {
        uart_send_string("ioctl_wrapper: file not opened\n");
        return -1;
    }

    int ret;
    uart_send_string("ioctl_wrapper\n");
    ret = vfs_ioctl(&get_current_thread() -> fds[fd], cmd, args);

    return ret;
}

void syscall_open(trapframe_t *tf, const char *pathname, int flags) {
    int fd = open_wrapper(pathname, flags);
    tf -> x[0] = fd;
    return;
}

void syscall_close(trapframe_t *tf, int fd) {
    int ret = close_wrapper(fd);
    tf -> x[0] = ret;
    return;
}

void syscall_write(trapframe_t *tf, int fd, const void *buf, size_t len) {
    int ret = write_wrapper(fd, buf, len);
    tf -> x[0] = ret;
    return;
}

void syscall_read(trapframe_t *tf, int fd, void *buf, size_t len) {
    int ret = read_wrapper(fd, buf, len);
    tf -> x[0] = ret;
    return;
}

void syscall_mkdir(trapframe_t *tf, const char *pathname, uint32_t mode) {
    int ret = mkdir_wrapper(pathname);
    tf -> x[0] = ret;
    return;
}

void syscall_mount(
    trapframe_t *tf,
    const char *src,
    const char *target,
    const char *fs_name,
    int flags,
    const void *data
) {
    int ret = mount_wrapper(target, fs_name);
    tf -> x[0] = ret;
}

void syscall_chdir(trapframe_t *tf, const char *path) {
    int ret = chdir_wrapper(path);
    tf -> x[0] = ret;
}

void syscall_lseek64(trapframe_t *tf, int fd, long offset, int whence) {
    long ret = lseek64_wrapper(fd, offset, whence);
    tf -> x[0] = ret;
}

void syscall_ioctl(trapframe_t *tf, int fd, uint64_t requests, ...) {
    uart_send_string("syscall_ioctl\n");
    int ret;
    va_list args;

    va_start(args, requests);
    ret = ioctl_wrapper(fd, requests, args);
    va_end(args);

    tf -> x[0] = ret;
}
