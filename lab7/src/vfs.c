#include "vfs.h"
#include "dev_uart.h"
#include "initramfs.h"
#include "memory.h"
#include "string.h"
#include "tmpfs.h"
#include "uart1.h"

struct mount *rootfs;
struct filesystem fs_reg[MAX_FS_REG];
struct file_operations reg_dev[MAX_DEV_REG];

int register_filesystem(struct filesystem *fs)
{
    for (int i = 0; i < MAX_FS_REG; i++) {
        if (!fs_reg[i].name) {
            fs_reg[i].name = fs->name;
            fs_reg[i].setup_mount = fs->setup_mount;
            return i;
        }
    }
    return -1;
}

struct filesystem *find_filesystem(const char *fs_name)
{
    for (int i = 0; i < MAX_FS_REG; i++) {
        if (!strcmp(fs_reg[i].name, fs_name)) {
            return &fs_reg[i];
        }
    }
    return 0;
}

int vfs_open(const char *pathname, int flags, struct file **target)
{
    struct vnode *node;
    // uart_printf("vfs_open: %s\n", pathname);
    if (vfs_lookup(pathname, &node) != 0 && (flags & O_CREAT)) { // if not found and O_CREAT flag is set
        // find parent dir
        // uart_printf("vfs_open: %s\n", pathname);
        int last_slash_idx = 0;
        for (int i = 0; i < strlen(pathname); i++) {
            if (pathname[i] == '/') {
                last_slash_idx = i;
            }
        }

        char dir_name[MAX_PATH_NAME + 1] = {};
        strcpy(dir_name, pathname);
        dir_name[last_slash_idx] = '\0';

        if (vfs_lookup(dir_name, &node) != 0) {
            // uart_printf("cannot ocreate \n");
            return -1; // non-exist parent dir
        }

        node->v_ops->create(node, &node, pathname + last_slash_idx + 1);
        *target = kmalloc(sizeof(struct file));
        node->f_ops->open(node, target);
        (*target)->flags = flags;
        return 0;
    }
    else {
        // uart_printf("vfs_open: %s\n", pathname);
        *target = kmalloc(sizeof(struct file));
        node->f_ops->open(node, target);
        (*target)->flags = flags;
        return 0;
    }

    return -1; // failed
}

int vfs_close(struct file *file)
{
    file->f_ops->close(file);
    return 0;
}

int vfs_write(struct file *file, const void *buf, size_t len) { return file->f_ops->write(file, buf, len); }

int vfs_read(struct file *file, void *buf, size_t len) { return file->f_ops->read(file, buf, len); }

int vfs_mkdir(const char *pathname)
{
    char dir_name[MAX_PATH_NAME] = {};
    char new_dir_name[MAX_PATH_NAME] = {};
    int last_slash_idx = 0;

    for (int i = 0; i < strlen(pathname); i++) {
        if (pathname[i] == '/') {
            last_slash_idx = i;
        }
    }

    memcpy(dir_name, pathname, last_slash_idx);
    strcpy(new_dir_name, pathname + last_slash_idx + 1);

    struct vnode *node;
    if (vfs_lookup(dir_name, &node) == 0) {
        node->v_ops->mkdir(node, &node, new_dir_name);
        return 0;
    }

    return -1;
}

int vfs_mount(const char *target, const char *filesystem)
{
    struct vnode *dirnode;
    struct filesystem *fs = find_filesystem(filesystem);
    if (fs == 0) {
        return -1; // filesystem not found
    }
    if (vfs_lookup(target, &dirnode) == -1) {
        return -1; // target not found
    }

    dirnode->mount = kmalloc(sizeof(struct mount));
    fs->setup_mount(fs, dirnode->mount);

    return 0;
}

int vfs_lookup(const char *pathname, struct vnode **target)
{
    // uart_printf("vfs_lookup: %s\n", pathname);
    if (strlen(pathname) == 0) {
        *target = rootfs->root;
        return 0;
    }

    struct vnode *dirnode = rootfs->root;
    char component_name[FILE_NAME_MAX + 1] = {};
    int c_idx = 0;
    for (int i = 1; i < strlen(pathname); i++) {
        if (pathname[i] == '/') {
            component_name[c_idx++] = 0;
            c_idx = 0;

            if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name) != 0)
                return -1;

            // redirect to new mounted filesystem
            if (dirnode->mount)
                dirnode = dirnode->mount->root;
        }
        else {
            component_name[c_idx++] = pathname[i];
        }
    }

    // uart_printf("component_name: %s\n", component_name);
    component_name[c_idx++] = 0;
    if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name) != 0)
        return -1;

    // redirect to new mounted filesystem
    if (dirnode->mount)
        dirnode = dirnode->mount->root;

    *target = dirnode;
    return 0;
}

void init_rootfs()
{
    // tmpfs
    int idx = register_tmpfs();
    rootfs = kmalloc(sizeof(struct mount));
    fs_reg[idx].setup_mount(&fs_reg[idx], rootfs);

    // initramfs
    vfs_mkdir("/initramfs");
    register_initramfs();
    vfs_mount("/initramfs", "initramfs");

    // dev
    vfs_mkdir("/dev");
    int uart_id = init_dev_uart();
    vfs_mknod("/dev/uart", uart_id);
}

char *get_abs_path(char *path, char *cur_working_dir)
{
    if (path[0] != '/') { // relative path
        char tmp[MAX_PATH_NAME];
        strcpy(tmp, cur_working_dir);
        if (strcmp(cur_working_dir, "/") != 0)
            strcat(tmp, "/");
        strcat(tmp, path);
        strcpy(path, tmp);
    }

    char abs_path[MAX_PATH_NAME + 1] = {};
    int idx = 0;
    for (int i = 0; i < strlen(path); i++) {
        if (path[i] == '/' && path[i + 1] == '.' && path[i + 2] == '.') {
            for (int j = idx; j >= 0; j--) {
                if (abs_path[j] == '/') {
                    abs_path[j] = 0;
                    idx = j;
                }
            }
            i += 2;
            continue;
        }

        if (path[i] == '/' && path[i + 1] == '.') {
            i++;
            continue;
        }

        abs_path[idx++] = path[i];
    }
    abs_path[idx] = 0;
    return strcpy(path, abs_path);
}

int register_dev(struct file_operations *f_ops)
{
    for (int i = 0; i < MAX_FS_REG; i++) {
        if (!reg_dev[i].open) {
            reg_dev[i] = *f_ops;
            return i;
        }
    }

    return -1;
}

int vfs_mknod(char *pathname, int id)
{
    // this function is used to create device file
    struct file *f = kmalloc(sizeof(struct file));

    vfs_open(pathname, O_CREAT, &f);
    f->vnode->f_ops = &reg_dev[id];
    vfs_close(f);
    return 0;
}