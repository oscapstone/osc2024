#include "sdhost.h"

#include "vfs.h"
#include "vfs_dev_framebuffer.h"
#include "vfs_dev_uart.h"
#include "vfs_initramfs.h"
#include "vfs_tmpfs.h"

#include "memory.h"
#include "string.h"
#include "uart1.h"
#include "sched.h"
#include "debug.h"

struct mount *rootfs;
struct filesystem reg_fs[MAX_FS_REG];
struct file_operations reg_dev[MAX_DEV_REG];
extern thread_t *curr_thread;

void rootfs_init()
{
    // sd_init
    sd_init();

    // tmpfs
    int idx = register_tmpfs();
    rootfs = kmalloc(sizeof(struct mount));
    reg_fs[idx].setup_mount(&reg_fs[idx], rootfs);

    // initramfs
    vfs_mkdir("/initramfs");
    register_initramfs();
    vfs_mount("/initramfs", "initramfs");

    // fat32
    vfs_mkdir("/boot");
    register_fat32();
    vfs_mount("/boot", "fat32");

    // // dev_fs
    vfs_mkdir("/dev");
    int uart_id = init_dev_uart();
    vfs_mknod("/dev/uart", uart_id);
    int framebuffer_id = init_dev_framebuffer();
    vfs_mknod("/dev/framebuffer", framebuffer_id);
}

int register_filesystem(struct filesystem *fs)
{
    for (int i = 0; i < MAX_FS_REG; i++)
    {
        if (!reg_fs[i].name)
        {
            reg_fs[i].name = fs->name;
            reg_fs[i].setup_mount = fs->setup_mount;
            reg_fs[i].sync = fs->sync; // ===================================================
            return i;
        }
    }
    return -1;
}

int vfs_lookup(const char *pathname, struct vnode **target)
{
    // int is_fat = 0;
    // if (!strcmp(pathname, "/boot"))
    // {
    //     is_fat = 1;
    //     uart_sendlinek("is boot...\n");
    // }

    // if no path input, return root
    if (strlen(pathname) == 0 || (!strcmp(&pathname[0], "/") && strlen(pathname) == 1))
    {
        *target = rootfs->root;
        // uart_sendlinek("path : %s is root \n", pathname);
        return 0;
    }
    struct vnode *dirnode = rootfs->root;
    char component_name[MAX_FILE_NAME + 1] = {};
    int c_idx = 0;
    // deal with directory
    for (int i = 1; i < strlen(pathname); i++)
    {
        if (pathname[i] == '/')
        {
            component_name[c_idx++] = 0;
            // if fs's v_ops error, return -1
            if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name) != 0)
                return -1;
            // redirect to mounted filesystem
            while (dirnode->mount)
            {
                dirnode = dirnode->mount->root;
                // if (is_fat)
                //     break;
            }
            c_idx = 0;
        }
        else
        {
            component_name[c_idx++] = pathname[i];
        }
    }

    // deal with file
    component_name[c_idx++] = 0;
    // if fs's v_ops error, return -1
    if (dirnode->v_ops->lookup(dirnode, &dirnode, component_name) != 0)
        return -1;
    // redirect to mounted filesystem
    while (dirnode->mount)
    {
        // if (is_fat)
        //     break;
        dirnode = dirnode->mount->root;
    }
    // return file's vnode
    *target = dirnode;

    return 0;
}

int register_dev(struct file_operations *fo)
{
    for (int i = 0; i < MAX_FS_REG; i++)
    {
        if (!reg_dev[i].open)
        {
            // return unique id for the assigned device
            reg_dev[i] = *fo;
            return i;
        }
    }
    return -1;
}

struct filesystem *find_filesystem(const char *fs_name)
{
    for (int i = 0; i < MAX_FS_REG; i++)
    {
        if (strcmp(reg_fs[i].name, fs_name) == 0)
        {
            return &reg_fs[i];
        }
    }
    return 0;
}

int vfs_mount(const char *target, const char *filesystem)
{
    struct vnode *dirnode;
    // search for the target filesystem
    struct filesystem *fs = find_filesystem(filesystem);
    if (!fs)
    {
        uart_sendlinek("vfs_mount cannot find filesystem\r\n");
        return -1;
    }

    if (vfs_lookup(target, &dirnode) == -1)
    {
        uart_sendlinek("vfs_mount cannot find dir\r\n");
        return -1;
    }
    else
    {
        // mount fs on dirnode
        dirnode->mount = kmalloc(sizeof(struct mount));
        // uart_sendlinek("fs name : %s\n",fs->name);
        fs->setup_mount(fs, dirnode->mount);
    }
    return 0;
}

// file ops
int vfs_mkdir(const char *pathname)
{
    struct vnode *node;
    if (vfs_lookup(pathname, &node) == 0)
    {
        WARING("Directory Exit!! : %s\n", pathname);
        return 0;
    }

    char dirname[MAX_PATH_NAME] = {};    // before add folder
    char newdirname[MAX_PATH_NAME] = {}; // after  add folder

    // search for last directory
    int last_slash_idx = 0;
    for (int i = 0; i < strlen(pathname); i++)
    {
        if (pathname[i] == '/')
        {
            last_slash_idx = i;
        }
    }

    memcpy(dirname, pathname, last_slash_idx);
    strcpy(newdirname, pathname + last_slash_idx + 1);

    // create new directory if upper directory is found
    // struct vnode *node;
    if (vfs_lookup(dirname, &node) == 0)
    {
        // node is the old dir, &node is new dir
        node->v_ops->mkdir(node, &node, newdirname);
        return 0;
    }

    uart_sendlinek("vfs_mkdir cannot find pathname");
    return -1;
}
// file ops
int vfs_open(const char *pathname, int flags, struct file **target)
{
    // 1. Lookup pathname
    // 3. Create a new file if O_CREAT is specified in flags and vnode not found
    // uart_sendlinek("\nhere!!!!!\n");
    struct vnode *node;
    if (vfs_lookup(pathname, &node) != 0 && (flags & O_CREAT))
    {
        // grep all of the directory path
        int last_slash_idx = 0;
        for (int i = 0; i < strlen(pathname); i++)
        {
            if (pathname[i] == '/')
            {
                last_slash_idx = i;
            }
        }

        char dirname[MAX_PATH_NAME + 1];
        strcpy(dirname, pathname);
        dirname[last_slash_idx] = 0;
        // update dirname to node
        if (vfs_lookup(dirname, &node) != 0)
        {
            uart_sendlinek("cannot ocreate no dir name\r\n");
            return -1;
        }
        // create a new file node on node, &node is new file, 3rd arg is filename
        node->v_ops->create(node, &node, pathname + last_slash_idx + 1);
        *target = kmalloc(sizeof(struct file));
        // attach opened file on the new node
        node->f_ops->open(node, target);
        (*target)->flags = flags;
        return 0;
    }
    else // 2. Create a new file handle for this vnode if found.
    {
        // attach opened file on the node

        *target = kmalloc(sizeof(struct file));
        node->f_ops->open(node, target);
        (*target)->flags = flags;
        return 0;
    }

    // lookup error code shows if file exist or not or other error occurs
    // 4. Return error code if fails
    return -1;
}

// file ops
int vfs_close(struct file *file)
{
    // 1. release the file handle
    // 2. Return error code if fails
    file->f_ops->close(file);
    return 0;
}

// file ops
int vfs_write(struct file *file, const void *buf, size_t len)
{
    // 1. write len byte from buf to the opened file.
    // 2. return written size or error code if an error occurs.
    return file->f_ops->write(file, buf, len);
}

// file ops
int vfs_read(struct file *file, void *buf, size_t len)
{
    // 1. read min(len, readable size) byte to buf from the opened file.
    // 2. block if nothing to read for FIFO type
    // 2. return read size or error code if an error occurs.
    return file->f_ops->read(file, buf, len);
}

// for device operations only
int vfs_mknod(char *pathname, int id)
{
    struct file *f;
    // create leaf and its file operations
    vfs_open(pathname, O_CREAT, &f);
    f->vnode->f_ops = &reg_dev[id];
    vfs_close(f);
    return 0;
}


int vfs_sync(struct filesystem *fs)
{
    return fs->sync(fs);
}

// void vfs_test()
// {
//     // test read/write
//     vfs_mkdir("/lll");
//     vfs_mkdir("/lll/ddd");
//     // test mount
//     vfs_mount("/lll/ddd", "tmpfs");

//     struct file* testfilew;
//     struct file *testfiler;
//     char testbufw[0x30] = "ABCDEABBBBBBDDDDDDDDDDD";
//     char testbufr[0x30] = {};
//     vfs_open("/lll/ddd/ggg", O_CREAT, &testfilew);
//     vfs_open("/lll/ddd/ggg", O_CREAT, &testfiler);
//     vfs_write(testfilew, testbufw, 10);
//     vfs_read(testfiler, testbufr, 10);
//     uart_sendline("%s",testbufr);

//     struct file *testfile_initramfs;
//     vfs_open("/initramfs/get_simpleexec.sh", O_CREAT, &testfile_initramfs);
//     vfs_read(testfile_initramfs, testbufr, 30);
//     uart_sendline("%s", testbufr);
// }

char *get_absolute_path(char *path, char *curr_working_dir)
{
    char absolute_path[MAX_PATH_NAME + 1] = {};
    int max_pathdeep = 10;
    struct dir_path *ctmp = kmalloc(sizeof(struct dir_path) * max_pathdeep);
    int deep = 0;

    if (path[0] == '/')
    {
        // uart_sendlinek("Input path is absolute_path: %s\n", path);
        strcpy(absolute_path, path);
    }
    else
    {
        // uart_sendlinek("Input path is relative_path: %s\n", path);
        strcpy(absolute_path, curr_working_dir);
        if (strcmp(curr_working_dir, "/") != 0)
        {
            strcat(absolute_path, "/");
        }
        strcat(absolute_path, path);
    }
    // uart_sendlinek("absolute_path: %s\n", absolute_path);
    // uart_sendlinek("strlen of bsolute_path: %d\n", strlen(absolute_path));

    int dir_namesize = 0;
    for (int i = 0; i < strlen(absolute_path); i++)
    {
        dir_namesize++;
        if (absolute_path[i + 1] == '/' || i + 1 == strlen(absolute_path))
        {
            ctmp[deep].size = dir_namesize;
            ctmp[deep].dir = &absolute_path[i + 1 - dir_namesize];
            dir_namesize = 0;
            // deep ++;
            // uart_sendlinek("ctmp[%d].dir : %s\n", deep, ctmp[deep].dir);
            // uart_sendlinek("size : %d\n", ctmp[deep].size);

            if (!strncmp(ctmp[deep].dir, "/..", 3))
            {
                // uart_sendlinek("Go back!!\n");
                deep > 0 ? deep-- : deep;
            }
            else if (!strncmp(ctmp[deep].dir, "/.", 2) || ctmp[deep].size <= 1)
            {
                // uart_sendlinek("Do Nothing!!\n");
            }
            else
            {
                deep++;
            }
        }
    }

    // uart_sendlinek("deep : %d\n", deep);
    int n = 0;
    char *cpath = &path[n];
    if (deep == 0)
    {
        // memset(path,0,MAX_PATH_NAME + 1);
        strncpy(cpath, "/\0", 2);
        kfree(ctmp);
        // uart_sendlinek("absolute_path: %s\n", path);
        return path;
    }

    for (int i = 0; i < deep; i++)
    {
        strncpy(cpath, ctmp[i].dir, ctmp[i].size);
        n += ctmp[i].size;
        cpath = &path[n];
        strncpy(cpath, "\0", 1);
    }

    // uart_sendlinek("absolute_path: %s\n", path);
    kfree(ctmp);
    return path;
}

void vfs_dump(struct vnode *_vnode, int level)
{
    // uart_sendlinek("In vfs_dump\n");
    displaylayer(level);
    if (_vnode->mount != 0)
    {
        // tmpfs_dump(_vnode->internal, level);
        _vnode->v_ops->dump(_vnode, level);
        while (_vnode->mount)
        {
            displaylayer(level);
            uart_sendlinek("  !!mount!!\n");
            _vnode = _vnode->mount->root;
        }
        vfs_dump(_vnode, level);
    }
    else
    {
        // tmpfs_dump(_vnode->internal, level);
        _vnode->v_ops->dump(_vnode, level);
    }
}

void vfs_ls()
{
    struct vnode *node;
    uart_sendlinek("In directory : %s\n", curr_thread->curr_working_dir);
    if (vfs_lookup(curr_thread->curr_working_dir, &node) == 0)
    {
        //uart_sendlinek("find!!\n");
        node->v_ops->ls(node);
    }
}

void vfs_cd(char *filepath)
{
    uart_sendlinek("Before change directory : %s\n", curr_thread->curr_working_dir);

    char abs_path[MAX_PATH_NAME];
    strcpy(abs_path, filepath);
    get_absolute_path(abs_path, curr_thread->curr_working_dir);
    // uart_sendlinek("abs_path : %s\n",abs_path);
    strcpy(curr_thread->curr_working_dir, abs_path);

    uart_sendlinek("In directory : %s\n", curr_thread->curr_working_dir);
}

void displaylayer(int level)
{
    for (int i = 0; i < level; i++)
    {
        uart_sendlinek("    ");
    }
}