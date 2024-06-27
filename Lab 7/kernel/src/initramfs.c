#include "initramfs.h"
#include "list.h"
#include "vfs.h"
#include "string.h"
#include "stat.h"
#include "initrd.h"
#include "uart.h"
#include "memory.h"


static int 
lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    vnode_ptr vnode = 0;
    list_for_each_entry(vnode, &dir_node->children, self) {
        if (!(str_cmp((byteptr_t) vnode->name, (byteptr_t) component_name))) {
            *target = vnode;
            return 0;
        }
    }
    return -1;
}


static int 
create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    return -1;
}


static int 
mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    return -1;
}


struct vnode_operations initramfs_v_ops = {
    lookup,
    create,
    mkdir,
};



static int 
write(struct file *file, const void *buf, int32_t len)
{
    uart_printf("[DEBUG] initramfs_write - len: %d\n", len);
    return 0;
}


static int 
read(struct file *file, void *buf, int32_t len)
{
    struct vnode *vnode = file->vnode;
    if (!S_ISREG(vnode->f_mode)) {
        uart_printf("[INFO] initramfs_read - not a regular file\n");
        return -1;
    }

    int min = (len > (vnode->content_size - file->f_pos - 1)) ? (vnode->content_size - file->f_pos - 1) : len; // -1 for EOF;
    if (min <= 0) { return -1; } // f_pos at EOF or len==0;
    file->f_pos += min;
    memory_copy((byteptr_t) buf, (byteptr_t) ((uint64_t) vnode->content + file->f_pos), min);

    return min;
}


static int 
open(struct vnode *file_node, struct file **target)
{
    return 0;
}


static int 
close(struct file *file)
{
    kfree((byteptr_t) file);
    return 0;
}


static long 
lseek64(struct file *file, long offset, int whence)
{
    return 0;
}


struct file_operations initramfs_f_ops = {
    write,
    read,
    open,
    close,
    lseek64,
};


static vnode_ptr dir_node;


static void 
_create_node(byteptr_t name,  byteptr_t content, uint32_t size)
{
    vnode_ptr target = 0;

    if (!lookup(dir_node, &target, name)) {
        uart_printf("[INFO] create_init - the %s file is already exist\n", name); return;
    }

    vnode_ptr new_vnode = vnode_create(name, S_IFREG);
    new_vnode->mount = dir_node->mount;
    new_vnode->v_ops = dir_node->v_ops;
    new_vnode->f_ops = dir_node->f_ops;
    new_vnode->parent = dir_node;

    new_vnode->content = content;
    new_vnode->content_size = size;

    list_add_tail(&new_vnode->self, &dir_node->children);
    dir_node->child_num += 1;
}   


static void
init_cpio_files()
{
    if (vfs_lookup("/initramfs", &dir_node)) { uart_printf("[INFO] init_cpio_files - fail to lookup dir\n"); }
    initrd_traverse(&_create_node);
}


static int 
setup_mount(struct filesystem *fs, struct mount *mount)
{
    //  should set mount->root as a obj before call setup_mount
    mount->root->mount = mount;
    mount->root->f_ops = &initramfs_f_ops;
    mount->root->v_ops = &initramfs_v_ops;
    mount->fs = fs;

    // clear the internal content
    INIT_LIST_HEAD(&mount->root->children);
    mount->root->child_num = 0;
    mount->root->content = 0;
    mount->root->content_size = 0;
    init_cpio_files();
    return 0;
}


struct filesystem *
initramfs_create()
{
    fs_ptr fs = (fs_ptr) kmalloc(sizeof(struct filesystem));
    fs->name = "initramfs";
    fs->setup_mount = &setup_mount;
    INIT_LIST_HEAD(&fs->list);
    return fs;
}