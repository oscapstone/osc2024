#include "kernel/initramfs.h"

struct file_operations initramfs_f_ops = {initramfs_write, initramfs_read, initramfs_open, initramfs_close, vfs_lseek64, initramfs_getsize};
struct vnode_operations initramfs_v_ops = {initramfs_lookup, initramfs_create, initramfs_mkdir};

int initramfs_register(){
    struct filesystem fs;
    fs.name = "initramfs";
    fs.setup_mount = initramfs_setup_mount;
    // return the index of the filesystem
    return register_filesystem(&fs);
}

int initramfs_setup_mount(struct filesystem *fs, struct mount *mount){
    mount->fs = fs;
    mount->root = initramfs_create_vnode(0, dir_t);
    
    struct initramfs_inode* root_inode = (struct initramfs_inode*)mount->root->internal;

    // add all files in initramfs to the root directory
    char *temp_addr = cpio_addr;
    int namesize;
    int filesize; 
    struct cpio_newc_header* header = (struct cpio_newc_header*)cpio_addr;
    int index = 0;
    if(string_comp_l(header->c_magic, "070701", 6) != 0){
        uart_puts("cpio magic value error\n");
        return -1;
    }
    // loop until the end of cpio
    while(string_comp((char*)(temp_addr + sizeof(struct cpio_newc_header)), "TRAILER!!!") != 0){
        header = (struct cpio_newc_header*)temp_addr;
        namesize = h2i(header->c_namesize, 8);
        filesize = h2i(header->c_filesize, 8);

        struct vnode *file_vnode = initramfs_create_vnode(0, file_t);
        struct initramfs_inode* file_inode = file_vnode->internal;

        file_inode->data_size = filesize;
        file_inode->name = (char*)(temp_addr + sizeof(struct cpio_newc_header));
        file_inode->data = (char*)(temp_addr + sizeof(struct cpio_newc_header) + namesize + align_offset((sizeof(struct cpio_newc_header) + namesize), 4));
        
        uart_puts("\n\n");
        uart_puts("initramfs_setup_mount: ");
        uart_puts(file_inode->name);
        uart_puts("\n\n");

        root_inode->entry[index++] = file_vnode;
        
        temp_addr += (sizeof(struct cpio_newc_header) + namesize + filesize + align_offset((sizeof(struct cpio_newc_header) + namesize), 4) + align_offset(filesize, 4));
    }
    return 0;
}
// create a vnode for initramfs
struct vnode* initramfs_create_vnode(struct mount* mount, enum node_type type){
    struct vnode* vnode = (struct vnode*)pool_alloc(4096);
    struct initramfs_inode* inode = (struct initramfs_inode*)pool_alloc(4096);

    vnode->mount = mount;
    vnode->v_ops = &initramfs_v_ops;
    vnode->f_ops = &initramfs_f_ops;
    
    memset(inode, 0, sizeof(struct initramfs_inode));
    inode->type = type;
    inode->data = (char*)pool_alloc(MAX_FILE_SIZE);

    vnode->internal = inode;

    return vnode;
}

my_uint64_t initramfs_getsize(struct vnode *vd){
    struct initramfs_inode* inode = (struct initramfs_inode*)vd->internal;
    return inode->data_size;
}

int initramfs_write(struct file *file, const void *buf, my_uint64_t len){
    // it's read-only
    return -1;
}

int initramfs_read(struct file *file, void *buf, my_uint64_t len){
    struct initramfs_inode* inode = (struct initramfs_inode*)file->vnode->internal;
    
    if(file->f_pos + len > inode->data_size){
        len = inode->data_size - file->f_pos;
        string_copy_n(buf, inode->data + file->f_pos, len);
    }
    else
        string_copy_n(buf, inode->data + file->f_pos, len);

    file->f_pos += len;

    return len;
}

int initramfs_open(struct vnode *file_node, struct file **target){
    (*target)->f_ops = file_node->f_ops;
    (*target)->f_pos = 0;
    (*target)->vnode = file_node;
    return 0;
}

int initramfs_close(struct file *file){
    pool_free(file);
    return 0;
}

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){
    struct initramfs_inode* dir_inode = (struct initramfs_inode*)dir_node->internal;

    for(int i = 0; i < MAX_RAMFS_ENTRY; i++){
        if(dir_inode->entry[i] != 0 && string_comp(((struct initramfs_inode*)(dir_inode->entry[i]->internal))->name, component_name) == 0){
            *target = dir_inode->entry[i];
            return 0;
        }
    }

    uart_puts("initramfs Lookup Error: Cannot find the vnode\n");
    return -1;
}

int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
    // it's read-only
    return -1;
}
int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    // it's read-only
    return -1;
}