#include "vfs.h"
#include "tmpfs.h"
#include "initramfs.h"
#include "string.h"
#include "memory.h"
#include "stat.h"
#include "sched.h"
#include "uart.h"


#define O_CREAT         00000100


mount_ptr rootfs, initramfs;
LIST_HEAD(vfs_list);


static const char *
_next_lvl_path(const char *src, char *dst, int size)
{
    for (int i = 0; i < size; ++i) {
        if (src[i] == 0) { dst[i] = 0; return 0; }
        else if (src[i] == '/') { dst[i] = 0; return ((char *) (uint64_t) src + i + 1); }
        else { dst[i] = src[i]; }
    }
    return 0;
}


static int 
_find_iter(const char *pathname, struct vnode **vnode)
{
    if (pathname[0] == '/') {
        *vnode = rootfs->root;
        return 1;
    }
    if (pathname[0] == '.') {
        if (pathname[1] == '.') {
            *vnode = current_task()->pwd->parent;
            return 3;
        }
        *vnode = current_task()->pwd;
        return 2;
    }
    *vnode = current_task()->pwd;
    return 0;
}


/* if the pathname='/dir1/dir2/text' -> parent=dir2  child=text */
static int 
_find_nodes(struct vnode **parent, struct vnode **child, const char *pathname, char *prefix)
{
    vnode_ptr target_node = 0;
    vnode_ptr itr = 0;

    int index = _find_iter(pathname, &itr);
    const char * _pathname = &pathname[index];  // pathname=../dir2/file   _pathname=dir2/file;

    /* if pathname='/' -> _pathname='' */
    if (!_pathname[0]) {
        *parent = rootfs->root;
        *child = rootfs->root;
        _next_lvl_path(_pathname, prefix, COMPONENT_SIZE); // update prefix
        return 0;
    }

    /* The itr will be a directory (root is the toppest) */
    while (1) {
        _pathname = _next_lvl_path(_pathname, prefix, COMPONENT_SIZE);
        if (itr->v_ops->lookup(itr, &target_node, prefix) == -1) {
            *parent = itr;
            *child = 0;
            return -1;
        }
        else {
            if (S_ISDIR(target_node->f_mode)) {
                if (!_pathname) {
                    /* encounter the last component */
                    *child = target_node;
                    *parent = target_node->parent;
                    return 0;
                }
                itr = target_node;
            }
            else if (S_ISREG(target_node->f_mode)) {
                *child = target_node;
                *parent = target_node->parent;
                return 0;
            }
        }
    }
    return 0;
}


static byteptr_t
dir_tok(byteptr_t s)
{
    static byteptr_t old_s = 0;
    if (!s) s = old_s;
    if (!s) return nullptr;

    // find begin
    while (*s == '/') { s++; }
    // reached the end
    if (*s == '\0') return nullptr;
    byteptr_t ret = s;
    // find end
    while (1) {
        if (*s == '\0') { old_s = s; return ret; }
        if (*s == '/') { *s = '\0'; old_s = s + 1; return ret; }
        s++;
    }
    return nullptr;
}



static int
_find_child_node(vnode_ptr *parent, char *path, vnode_ptr *child, char **name)
{
    // uart_printf("_find_child_node -  parent->name: %s, path: %s\n", (*parent)->name, path);

    if (*parent == 0)
        *parent = (path[0] == '/') ? rootfs->root : current_task()->pwd;
    
    *name = dir_tok(path);

    if ((*name)[0] == '.') {
        if ((*name)[1] == '.') *child = current_task()->pwd->parent;
        else *child = current_task()->pwd;
        return 0;
    }

    if ((*name) == 0) 
        return 1; // end
    
    if ((*parent)->v_ops->lookup(*parent, child, *name) == 0) {
        return 0; // found
    }
    return -1;    // not found
}



fs_ptr vfs_get(const char *name)
{
    struct filesystem *fs;
    list_for_each_entry(fs, &vfs_list, list) {
        if (!str_cmp((byteptr_t) fs->name, (byteptr_t) name)) return fs;
    }
    return 0;
}


int32_t
vfs_register(fs_ptr fs)
{
    if (!vfs_get(fs->name)) {
        list_add_tail(&fs->list, &vfs_list);
        return 0;
    }
    return -1;
}


int 
vfs_mkdir(const char *pathname)
{
    uart_printf("[DEBUG] vfs_mkdir: %s, ", pathname);

    vnode_ptr node, parent = 0;
    char buf[128];
    char * tok;

    int code = _find_child_node(&parent, pathname, &node, &tok);
    // uart_printf("node: 0x%x, name: %s, code: %d, buf: %s\n", node, node->name, code, tok);

    while (code < 1) {
        if (code == -1) { parent->v_ops->mkdir(parent, &node, tok); }

        parent = node;
        code = _find_child_node(&parent, 0, &node, &tok);
        // uart_printf("node: 0x%x, name: %s, code: %d, buf: %s\n", node, node->name, code, tok);
    }
    return 0;
}


int 
vfs_lookup(const char *pathname, struct vnode **target)
{
    char child_name[COMPONENT_SIZE];
    vnode_ptr parent = 0, child = 0;
    _find_nodes(&parent, &child, pathname, child_name);

    if (child) {
        *target = child;
        return 0;
    }
    return -1;
}


static
vnode_ptr _find(vnode_ptr parent, char *name) {}


static int
_lookup(const char *pathname, vnode_ptr* target)
{
    vnode_ptr node, parent = 0;
    char buf[128];
    char * tok;

    if (parent == 0) {
        if (pathname[0] == '/') {
            pathname += 1;
            parent = rootfs->root;
        }
        else
            parent = current_task()->pwd;
    }
    
    tok = dir_tok(pathname);
    int code = parent->v_ops->lookup(parent, &node, tok);

    uart_printf("node: 0x%x, name: %s, code: %d, tok: %s\n", node, node->name, code, tok);

    while (code == 0) {
        parent = node;
        tok = dir_tok(0);
        code = parent->v_ops->lookup(parent, &node, tok);
        uart_printf("node: 0x%x, name: %s, code: %d, tok: %s\n", node, node->name, code, tok);
    }


    // int code = _find_child_node(&parent, pathname, &node, &tok);
    // *target = node;
    // uart_printf("node: 0x%x, name: %s, code: %d, buf: %s\n", node, node->name, code, tok);

    // while (code == 0) {
    //     current_task()->pwd = node;
    //     parent = node;
    //     code = _find_child_node(&parent, 0, &node, &tok);
    //     uart_printf("node: 0x%x, name: %s, code: %d, buf: %s\n", node, node->name, code, tok);
    // }
    // if (code == 1) {
    //     *target = parent;
    //     uart_printf("--- %s ---\n", parent->name);
    // }
    return code;
}


int
vfs_chdir(const char *pathname)
{
    vnode_ptr node, parent = 0;
    char buf[128];
    char * tok;

    int code = _find_child_node(&parent, pathname, &node, &tok);
    // uart_printf("node: 0x%x, name: %s, code: %d, buf: %s\n", node, node->name, code, tok);

    while (code == 0) {
        uart_printf("[DEBUG] vfs_chdir: %s\n", node->name);
        current_task()->pwd = node;
        parent = node;
        code = _find_child_node(&parent, 0, &node, &tok);
        // uart_printf("node: 0x%x, name: %s, code: %d, buf: %s\n", node, node->name, code, tok);
    }

    return 0;

    // vnode_ptr node = 0;
    // if (vfs_lookup(pathname, &node) == -1) {
    //     uart_printf("[INFO] vfs_chdir - fail to loopup the dir:%s\n", pathname);
    //     return -1;
    // }
    // current_task()->pwd = node;
    // uart_printf("[DEBUG] vfs_chdir - current->pwd: 0x%x, %s\n", node, node->name);
    // return 0;
}


int 
vfs_mount(const char *target, const char *filesystem)
{
    struct filesystem *fs = vfs_get(filesystem);
    if (!fs) { uart_printf("[INFO] vfs_mount - Error! fail to get fs\n"); return -1; }

    // find the vnode
    struct vnode *vnode;
    if (vfs_lookup(target, &vnode) == -1) { uart_printf("[INFO] vfs_mount - Error! fail to lookup\n"); return -2; }

    // check whether it's a dir
    if (!S_ISDIR(vnode->f_mode)) { uart_printf("[INFO] vfs_mount - Error! the target is not a directory\n"); return -1; }

    // link the root of the mount
    mount_ptr new_mount = (mount_ptr) kmalloc(sizeof(mount_t));
    new_mount->root = vnode;
    new_mount->fs = vnode->mount->fs;
    vnode->mount = new_mount;

    // run the setup
    new_mount->fs->setup_mount(fs, new_mount);

    return 0;
}


int 
vfs_open(const char *pathname, int flags, struct file **target)
{
    uart_printf("[DEBUG] vfs_open: %s, %d\n", pathname, sizeof(file_t));

    int res = 0;
    vnode_ptr target_node = 0;
    res = vfs_lookup(pathname, &target_node);

    if (res == -1 && !(flags & O_CREAT)) {
        /* can't lookup and without O_CREAT flag */
        uart_printf("[INFO] [vfs_open] fail to open the %s\n", pathname);
        return -1;
    }

    *target = (struct file *) kmalloc(sizeof(file_t));
    (*target)->flags = flags;
    (*target)->f_ops = target_node->f_ops;
    (*target)->f_pos = 0;

    if (!res) {
        (*target)->vnode = target_node;
        return 0;
    }

    char child_name[COMPONENT_SIZE];
    vnode_ptr parent = 0, child = 0;
    _find_nodes(&parent, &child, pathname, child_name);
    if (!child) {
        parent->v_ops->create(parent, &child, child_name);
        (*target)->vnode = child;
        return 0;
    }

    return -1;
}


int 
vfs_close(struct file *file)
{
    return file->vnode->f_ops->close(file);
}


int 
vfs_write(struct file *file, const void *buf, int32_t len)
{
    return file->vnode->f_ops->write(file, buf, len);
}


int vfs_read(struct file *file, void *buf, int32_t len)
{
    return file->vnode->f_ops->read(file, buf, len);
}


static void 
_init_rootfs()
{
    if (vfs_register(tmpfs_create())) {
        uart_printf("[INFO] init_rootfs - Error! fail to register tmpfs\n");
    }

    rootfs = (mount_ptr) kmalloc(sizeof(mount_t));

    fs_ptr fs = vfs_get("tmpfs");
    if (!fs) {
        uart_printf("[INFO] init_rootfs - Error! fail to get tmpfs\n");
        return;
    }

    rootfs->fs = fs;
    rootfs->root = vnode_create("", S_IFDIR);
    rootfs->fs->setup_mount(rootfs->fs, rootfs);
}


static void
_init_initramfs()
{   
    vnode_ptr initramfs_root = 0;
    if (vfs_lookup("/initramfs", &initramfs_root)) { uart_printf("[INFO] init_initramfs - fail to lookup /initramfs\n"); }
    if (vfs_register(initramfs_create())) { uart_printf("[INFO] init_initramfs - Error! fail to register initramfs\n"); }
    fs_ptr fs = vfs_get("initramfs");
    if (!fs) { uart_printf("[INFO] init_initramfs - Error! fail to get initramfs\n"); return; }
    
    initramfs = (mount_ptr) kmalloc(sizeof(mount_t));
    initramfs->fs = fs;
    initramfs->root = initramfs_root;
    initramfs->fs->setup_mount(initramfs->fs, initramfs);
}


void 
vfs_init()
{
    _init_rootfs();
    vfs_mkdir("/initramfs");
    _init_initramfs();
}



vnode_ptr
vnode_create(const byteptr_t name, uint32_t flags)
{
    // uart_printf("[DEBUG] vnode_create: %s\n", name);

    vnode_ptr vnode = (vnode_ptr) kmalloc(sizeof(vnode_t));

    INIT_LIST_HEAD(&vnode->children);
    INIT_LIST_HEAD(&vnode->self);

    vnode->child_num = 0;
    vnode->parent = 0;

    int32_t name_len = (int32_t) str_len(name);
    vnode->name = (byteptr_t) kmalloc(name_len);
    memory_copy((byteptr_t) vnode->name, (byteptr_t) name, name_len);

    vnode->f_mode = flags;
    vnode->content = 0;
    vnode->content_size = 0;

    return vnode;
}



static int
vnode_path(vnode_ptr node, char *buf)
{
    if (node->parent == 0) {
        buf[0] = 0;
        return 0;
    }

    int index = vnode_path(node->parent, buf);
    buf[index++] = '/';
    int len = str_len(node->name);
    memory_copy(&(buf[index]), node->name, len);
    buf[index + len] = 0;

    return index + len;
}


static
vfs_pwd()
{
    char buf[128];
    vnode_path(current_task()->pwd, buf);
    uart_printf("[DEBUG] vfs_pwd - %s\n", buf);
}


void
vfs_demo()
{
    vfs_mkdir("/doc/abc");
    vfs_chdir("/doc");
    vfs_chdir("abc");
    vfs_pwd();

    vfs_mkdir("def");
    vfs_pwd();

    vfs_chdir("/doc");
    vfs_pwd();
   
    vfs_chdir("abc");
    vfs_pwd();
    
    vfs_chdir("..");
    vfs_pwd();

    if (vfs_mkdir("../home") == -1) { uart_printf("[INFO] fail to mkdir /home\n"); }
    if (vfs_mkdir("./os") == -1) { uart_printf("[INFO] fail to mkdir /doc/os\n"); }

    file_ptr f1;
    vfs_open("../home/f1", O_CREAT, &f1);
    char buf1[10] = "345678abc";
    vfs_write(f1, buf1, 10);
    vfs_close(f1);

    vfs_chdir("..");
    vfs_chdir("/home");
    vfs_pwd();

    char buf2[10] = {0};
    vfs_open("./f1", 0, &f1);
    vfs_read(f1, buf2, 8);
    uart_printf("[INFO] read file /home/f1: %s\n", buf2);
    vfs_close(f1);

    if (!vfs_mount("/doc/abc/def", "tmpfs")) { uart_printf("[INFO] vfs_mount /doc/abc/def success\n"); }
}