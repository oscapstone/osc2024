#include "tmpfs.h"
#include "stat.h"
#include "list.h"
#include "uart.h"
#include "memory.h"
#include "string.h"


static int 
lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    vnode_ptr vnode = 0;
    list_for_each_entry(vnode, &dir_node->children, self) {
        if (!(str_cmp(vnode->name, (byteptr_t) component_name))) {
            *target = vnode;        // the output is here
            return 0;
        }
    }
    *target = 0;
    return -1;
}


static int 
create(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    if (!lookup(dir_node, target, component_name)) {
        uart_printf("[INFO] tmpfs_create - the %s file is already exist\n", component_name);
        return -1;
    }

    vnode_ptr new_vnode = vnode_create((byteptr_t) component_name, S_IFREG);
    new_vnode->mount = dir_node->mount;
    new_vnode->v_ops = dir_node->v_ops;
    new_vnode->f_ops = dir_node->f_ops;
    new_vnode->parent = dir_node;

    list_add_tail(&new_vnode->self, &dir_node->children);
    dir_node->child_num += 1;

    *target = new_vnode;            // the output is here
    return 0;
}


static int 
mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    uart_printf("[DEBUG] tmpfs_mkdir - %s, dir: 0x%x, dir->name: %s\n", component_name, dir_node, dir_node->name);
    if (!lookup(dir_node, target, component_name)) {
        uart_printf("[INFO] tmpfs_mkdir - the %s directory is already exist\n", component_name);
        return -1;
    }

    vnode_ptr new_vnode = vnode_create((byteptr_t) component_name, S_IFDIR);
    new_vnode->mount = dir_node->mount;
    new_vnode->v_ops = dir_node->v_ops;
    new_vnode->f_ops = dir_node->f_ops;
    new_vnode->parent = dir_node;

    list_add_tail(&new_vnode->self, &dir_node->children);
    dir_node->child_num += 1;

    *target = new_vnode;             // the output is here
    return 0;
}


vnode_ops_t tmpfs_v_ops = {
    lookup,
    create,
    mkdir,
};


static int 
write(struct file *file, const void *buf, int32_t len)
{
    uart_printf("[DEBUG] tmpfs_write - len: %d\n", len);
    
    struct vnode *vnode = file->vnode;
    if (!S_ISREG(vnode->f_mode)) {
        uart_printf("[INFO] tmpfs_write - not a regular file\n");
        return 0;
    }

    if ((file->f_pos + len) >= vnode->content_size ) { 
        // enlarge content, +1 for EOF
        byteptr_t new_content = (byteptr_t) kmalloc((uint32_t) (file->f_pos) + len + 1);
        
        memory_copy(new_content, vnode->content, vnode->content_size); // copy origin data;
        if (vnode->content) { kfree(vnode->content); }
    
        vnode->content = new_content;
        vnode->content_size = file->f_pos + len + 1;
    }

    memory_copy((byteptr_t) ((uint64_t) vnode->content + file->f_pos), (byteptr_t) buf, len);
    file->f_pos += len;

    return len;
}


static int 
read(struct file *file, void *buf, int32_t len)
{
    struct vnode *vnode = file->vnode;
    if (!S_ISREG(vnode->f_mode)) {
        uart_printf("[INFO] tmpfs_read - not a regular file\n");
        return -1;
    }

    int min = (len > (vnode->content_size - file->f_pos - 1)) ? (vnode->content_size - file->f_pos - 1) : len; // -1 for EOF;
    if (min <= 0) { return -1; } // f_pos at EOF or len==0;
    
    uart_printf("[INFO] tmpfs_read - len: %d, f_pos: %d\n", min, file->f_pos);
    
    memory_copy((byteptr_t) buf, (byteptr_t) ((uint64_t) vnode->content + file->f_pos), min);
    file->f_pos += min;

    return min;
}


static int
open(struct vnode *file_node, struct file **target)    // TODO
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
lseek64(struct file *file, long offset, int whence)     // TODO
{
    return 0;
}


file_ops_t tmpfs_f_ops = {
    write,
    read,
    open,
    close,
    lseek64,
};



static int32_t 
setup_mount(fs_ptr fs, mount_ptr mount)
{
    uart_line("setup_mount");
 
    mount->fs = fs;

    //  should set mount->root as a obj before call setup_mount
    vnode_ptr root = mount->root;

    root->mount = mount;
    root->f_ops = &tmpfs_f_ops;
    root->v_ops = &tmpfs_v_ops;

    // clear the internal content
    INIT_LIST_HEAD(&root->children);
    root->child_num = 0;
    root->content = 0;
    root->content_size = 0;
    
    return 0;
}


fs_ptr
tmpfs_create()
{
    fs_ptr fs = (fs_ptr) kmalloc(sizeof(filesystem_t));
    fs->name = "tmpfs";
    fs->setup_mount = &setup_mount;
    INIT_LIST_HEAD(&fs->list);

    return fs;
}