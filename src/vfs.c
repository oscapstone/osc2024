#include "vfs.h"
#include "uart.h"
#include "mm.h"
#include "string.h"
#include "stddef.h"
#include "tmpfs.h"
#include "initramfs.h"

struct filesystem *filesystems[NR_FILESYSTEM];
struct mount *rootfs;

int register_filesystem(struct filesystem *fs)
{
    for (int i = 0; i < NR_FILESYSTEM; i++) {
        if (!filesystems[i]) {
            filesystems[i] = fs;
#ifdef VFS_DEBUG
            printf("    [register_filesystem] File system %s registered idx %d\n", filesystems[i]->name, i);
#endif
            return 1; // 1 means success
        }
    }
    printf("    [register_filesystem] No space for more filesystem\n");
    return 0; // 0 means error
}


/**
 * Mount given file system on target path (path should be absolute path).
 * TODO: Deal with '/' path
 */
int vfs_mount(const char* target, const char* filesystem)
{
    struct filesystem *fs = NULL;
    struct vnode *target_node = NULL;
    struct mount *mount;

    for (int i = 0; i < NR_FILESYSTEM; i++) {
        if (filesystems[i] && !strcmp(filesystems[i]->name, filesystem)) {
            fs = filesystems[i];
            break;
        }
    }
    if (!fs) {
        printf("    [vfs_mount] File system not found\n");
        return 0;
    }

    /* Find the target vnode */
    if (!vfs_lookup(target, &target_node)) {
        printf("    [vfs_mount] vfs_lookup failed, can't find the target vnode\n");
        return 0;
    }

    /* Create mount to the target vnode */
    mount = (struct mount*)kmalloc(sizeof(struct mount));
    if (!fs->setup_mount(fs, mount)) {
        printf("    [vfs_mount] File system setup_mount failed\n");
        return 0;
    }

    target_node->mount = mount; // Jump to the mounted file system by target_node->mount->root
    return 1;
}

/* Walk from rootfs, find the target vnode. pathname must be absolute path */
int vfs_lookup(const char *pathname, struct vnode **target)
{
    struct vnode *dir = rootfs->root;
    char path[MAX_PATH_LEN];
    int i, idx = 0;

    if (strlen(pathname) == 0 || (strlen(pathname) == 1 && pathname[0] == '/')) { // return root
        *target = rootfs->root;
        return 1;
    }

    for (i = 1; i < strlen(pathname); i++) { // i = 1, assuem the first char is '/'
        if (pathname[i] == '/') { // the current path[] is directory name
            path[idx++] = '\0';
            if (!dir->v_ops->lookup(dir, &dir, path)) {
#ifdef VFS_DEBUG
                printf("    [vfs_lookup] lookup failed, %s not found\n", path);
#endif
                return 0;
            }

            /* Redirect to mounted file system. */
            while (dir->mount)
                dir = dir->mount->root;
            idx = 0;
        } else
            path[idx++] = pathname[i];
    }
    path[idx] = '\0';
    if (!dir->v_ops->lookup(dir, &dir, path)) { // last lookup, should be the target
#ifdef VFS_DEBUG
        printf("    [vfs_lookup] lookup failed, %s not found\n", path);
#endif
        return 0;
    }

    /* Redirect to mounted file system. */
    while (dir->mount)
        dir = dir->mount->root;
    *target = dir;

    return 1;
}

/* Lookup path to find target vnode, then create struct file. */
int vfs_open(const char* pathname, int flags, struct file** target)
{
    struct vnode *node, *target_node;
    char path[MAX_PATH_LEN];
    int i, idx = 0;

#ifdef VFS_DEBUG
    printf("    [vfs_open] Open file %s\n", pathname);
#endif

    /* Lookup path name*/
    if (!vfs_lookup(pathname, &node)) {
        /* If vnode not found and O_CREAT is specified in flags, create a new file */
        if (flags & O_CREAT) {
#ifdef VFS_DEBUG
            printf("    [vfs_open] vfs_lookup %s failed, but O_CREAT is specified\n", pathname);
#endif
            /* Find the directory. First get the directory path */
            for (i = 0; i < strlen(pathname); i++) {
                if (pathname[i] == '/')
                    idx = i; // idx is the last '/' index
                path[i] = pathname[i];
            }
            path[idx] = '\0';

            /* Lookup the directory */
            if (!vfs_lookup(path, &node)) {
                printf("    [vfs_open] directory %s not found, can't create file\n", path);
                return 0;
            }

            /* node is for directory now. Create a new vnode in the directory node */
            node->v_ops->create(node, &target_node, pathname + idx + 1);
            *target = (struct file*)kmalloc(sizeof(struct file));
            /* Attach the file to vnode */
            target_node->f_ops->open(target_node, target);
            (*target)->flags = flags;

            return 1;
        }
#ifdef VFS_DEBUG
        printf("    [vfs_open] vnode not found and O_CREAT not specified\n");
#endif
        return 0;
    }

    /* Create a new file handle for this vnode */
    *target = (struct file*)kmalloc(sizeof(struct file));
    node->f_ops->open(node, target);
    (*target)->flags = flags;
    return 1;
}

/* close always success. it just free the structure file. */
int vfs_close(struct file* file)
{
    /* Release the file handle */
    file->f_ops->close(file);
    return 1;
}

/* return 0 if error. (len should not be 0) */
int vfs_write(struct file* file, const void* buf, size_t len)
{
    if (!len)
        printf("    [vfs_write] len should not be zero\n");
    return file->f_ops->write(file, buf, len);
}

int vfs_read(struct file* file, void* buf, size_t len)
{
    if (!len)
        printf("    [vfs_read] len should not be zero\n");
    return file->f_ops->read(file, buf, len);
}

void rootfs_init(void)
{
    register_filesystem(&tmpfs_filesystem);
    register_filesystem(&initramfs_filesystem);

    /* Setup "/" path to tmpfs file system manually */
    rootfs = (struct mount*) kmalloc(sizeof(struct mount));
    filesystems[0]->setup_mount(filesystems[0], rootfs);

    /* Mount initramfs on path "/initramfs" */
    vfs_mkdir("/initramfs");
    vfs_mount("/initramfs", "initramfs");
}

/**
 * Get the absolute path from the current directory path and path.
 * The absolute path is stored in path.
 */
void get_absolute_path(char *path, char *current_path)
{
    char abs_path[MAX_PATH_LEN];
    int i, idx = 0;

    /* Check whether path is relative path or not. */
    if (path[0] != '/') {
        /* Concatenate current path and current path: abs_path = current + "/" + path */
        strcpy(abs_path, current_path);
        if (strcmp(current_path, "/")) // if current_path is not root, we should add '/'
            strcat(abs_path, "/");
        strcat(abs_path, path);
    } else // path is absolute path, just copy it
        strcpy(abs_path, path);

    /* Deals with abs_path with "..", ".", then store it to path. */
    for (i = 0; i < strlen(abs_path); i++) {
        /* Meet "/..", go back to upper directory. */
        if (abs_path[i] == '/' && abs_path[i + 1] == '.' && abs_path[i + 2] == '.') {
            /* e.g. path="/aaa/bbb", abs_path="/aaa/bbb/../ccc", idx=8, i=8 */
            while (idx >= 0 && path[idx] != '/')
                idx--;
            /* Now, path="/aaa/", we just continue */
            i += 2;
            continue;
        }

        /* Ignore "/." */
        if (abs_path[i] == '/' && abs_path[i + 1] == '.') {
            i++;
            continue;
        }

        /* copy abs_path[i] to path[idx] */
        path[idx++] = abs_path[i];
    }
    path[idx] = '\0';
    return;
}

/* make directory to path*/
int vfs_mkdir(const char* path)
{
    char dir_name[MAX_PATH_LEN], new_dir_name[MAX_PATH_LEN];
    unsigned int last_slash = 0, i;
    struct vnode *vnode;

    for (i = 0; i < strlen(path); i++)
        if (path[i] == '/')
            last_slash = i;
    
    memcpy(dir_name, path, last_slash);
    strcpy(new_dir_name, (path + last_slash + 1));

    if (vfs_lookup(dir_name, &vnode)) {
        vnode->v_ops->mkdir(vnode, &vnode, new_dir_name);
        return 1;
    }

    printf("    [vfs_mkdir] can't find the upper directory\n");
    return 0;
}