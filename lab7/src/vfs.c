#include "vfs.h"
#include "mm.h"
#include "sched.h"
#include "string.h"
#include "vfs_tmpfs.h"

struct mount *rootfs;
struct filesystem fs_list[MAX_FS];
struct file_operations dev_list[MAX_DEV];

void vfs_init()
{
    rootfs = (struct mount *)kmalloc(sizeof(struct mount));
    int idx = register_tmpfs();
    fs_list[idx].setup_mount(&fs_list[idx], rootfs);
}

int register_filesystem(struct filesystem *fs)
{
    for (int i = 0; i < MAX_FS; i++) {
        if (!fs_list[i].name) {
            fs_list[i].name = fs->name;
            fs_list[i].setup_mount = fs->setup_mount;
            return i;
        }
    }
    return -1;
}

int vfs_open(const char *pathname, int flags, struct file **target)
{
    struct vnode *vnode;

    // Create a new file if the vnode does not exist and O_CREAT is set
    if (vfs_lookup(pathname, &vnode) != 0 && flags & O_CREAT) {
        int pos = 0;
        for (int i = 0; i < strlen(pathname); i++)
            if (pathname[i] == '/')
                pos = i;

        char dirname[PATH_MAX] = { 0 };
        strncpy(dirname, pathname, pos);
        const char *filename = (pathname + pos + 1);

        if (vfs_lookup(dirname, &vnode) != 0)
            return -1;

        vnode->v_ops->create(vnode, &vnode, filename);
    }

    (*target) = kmalloc(sizeof(struct file));
    (*target)->flags = flags;
    vnode->f_ops->open(vnode, target);
    return 0;
}

int vfs_close(struct file *file)
{
    return file->f_ops->close(file);
}

int vfs_read(struct file *file, void *buf, size_t len)
{
    return file->f_ops->read(file, buf, len);
}

int vfs_write(struct file *file, const void *buf, size_t len)
{
    return file->f_ops->write(file, buf, len);
}

int vfs_mkdir(const char *pathname)
{
    char dirname[PATH_MAX] = { 0 };
    char basename[PATH_MAX] = { 0 };

    int pos = 0;
    for (int i = 0; i < strlen(pathname); i++)
        if (pathname[i] == '/')
            pos = i;

    strncpy(dirname, pathname, pos);
    strncpy(basename, pathname + pos + 1, strlen(pathname + pos + 1));

    struct vnode *vnode;
    if (vfs_lookup(dirname, &vnode) == 0) {
        vnode->v_ops->mkdir(vnode, &vnode, basename);
        return 0;
    }

    return -1;
}

int vfs_mount(const char *target, const char *filesystem)
{
    struct vnode *dir_node;
    struct filesystem *fs;
    for (int i = 0; i < MAX_FS; i++)
        if (strcmp(fs_list[i].name, filesystem) == 0)
            fs = &fs_list[i];

    if (!fs || vfs_lookup(target, &dir_node) != 0)
        return -1;

    dir_node->mount = kmalloc(sizeof(struct mount));
    fs->setup_mount(fs, dir_node->mount);
    return 0;
}

int vfs_lookup(const char *pathname, struct vnode **target)
{
    if (strlen(pathname) == 0) {
        *target = rootfs->root;
        return 0;
    }

    struct vnode *node = rootfs->root;
    char component[PATH_MAX] = { 0 };
    int idx = 0;

    for (int i = 1; i < strlen(pathname); i++) {
        if (pathname[i] == '/') {
            component[idx] = '\0';
            if (node->v_ops->lookup(node, &node, component) != 0)
                return -1;
            while (node->mount)
                node = node->mount->root;
            idx = 0;
        } else {
            component[idx++] = pathname[i];
        }
    }
    component[idx] = '\0';

    if (node->v_ops->lookup(node, &node, component) != 0)
        return -1;

    while (node->mount)
        node = node->mount->root;

    *target = node;
    return 0;
}

char *realpath(const char *path, char *resolved_path)
{
    char *cwd = get_current()->cwd;
    if (path[0] == '.' && path[1] == '/') {
        strncpy(resolved_path, cwd, strlen(cwd));
        int is_root = (strlen(cwd) == 1 && cwd[0] == '/');
        strcat(resolved_path, is_root ? path + 2 : path + 1);
        return resolved_path;
    } else if (path[0] == '.' && path[1] == '.') {
        int len = strlen(cwd);
        for (int i = len - 1; i >= 0; i--) {
            if (cwd[i] == '/') {
                strncpy(resolved_path, cwd, i);
                resolved_path[i] = '\0';
                strcat(resolved_path, path + 2);
                return resolved_path;
            }
        }
    } else {
        strncpy(resolved_path, path, strlen(path));
        return resolved_path;
    }
}
