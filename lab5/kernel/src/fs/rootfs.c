
#include "rootfs.h"
#include "mm/mm.h"
#include "utils/printf.h"
#include "utils/utils.h"

static int write(FS_FILE *file, const void *buf, size_t len);
static int read(FS_FILE *file, void *buf, size_t len);
static int open(FS_VNODE *file_node, FS_FILE **target);
static int close(FS_FILE *file);
static long lseek64(FS_FILE *file, long offset, int whence);

FS_FILE_OPERATIONS rootfs_f_ops = {
    write,
    read,
    open,
    close,
    lseek64
};

static int lookup(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
static int create(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
static int mkdir(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
FS_VNODE_OPERATIONS rootfs_v_ops = {
    lookup,
    create,
    mkdir
};


static int setup_mount(FS_FILE_SYSTEM* fs, FS_MOUNT* mount) {
    mount->root->mount = mount;
    mount->root->f_ops = &rootfs_f_ops;
    mount->root->v_ops = &rootfs_v_ops;
    mount->fs = fs;

    link_list_init(&mount->root->childs);
    mount->root->child_num = 0;
    mount->root->content = NULL;
    mount->root->content_size = 0;
    NS_DPRINT("[FS] rootfs. (or called tempfs in lab) setuped.\n");
    return 0;
}


FS_FILE_SYSTEM* rootfs_create() {
    FS_FILE_SYSTEM* fs = kmalloc(sizeof(FS_FILE_SYSTEM));
    fs->name = "rootfs";
    fs->setup_mount = &setup_mount;
    link_list_init(&fs->list);
    return fs;
}

static int write(FS_FILE *file, const void *buf, size_t len) {
    FS_VNODE* vnode = file->vnode;
    if (!S_ISREG(vnode->mode)) {
        printf("[FS] rootfs: write(): not a regular file\n");
        return -1;
    }
    if (vnode->content_size <= file->pos + len) {
        size_t new_size = file->pos + len + 1;
        void* new_content = kzalloc(new_size);
        if (vnode->content) {
            memcpy(vnode->content, new_content, vnode->content_size);
            kfree(vnode->content);
        }
        vnode->content = new_content;
        vnode->content_size = new_size;
    }

    memcpy(buf, (void*)((char*)vnode->content + file->pos), len);
    file->pos += len;

    return len;
}

static int read(FS_FILE *file, void *buf, size_t len) {
    FS_VNODE* vnode = file->vnode;
    if (!S_ISREG(vnode->mode)) {
        printf("[FS] read(): not a regular file\n");
        return -1;
    }

    int min = (len > vnode->content_size - file->pos - 1) ? vnode->content_size - file->pos - 1 : len;
    if (min == 0) {
        return -1;
    }
    memcpy(buf, (void*)((char*)vnode->content + file->pos), min);
    file->pos += min;
    return min;
}

static int open(FS_VNODE *file_node, FS_FILE **target) {
    // virtual fs doesn't need to open
    return 0;
}

static int close(FS_FILE *file) {
    // virtual fs doesn't need to close
    return 0;
}

static long lseek64(FS_FILE *file, long offset, int whence) {
    return 0;
}

static int lookup(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    FS_VNODE* vnode = NULL;
    NS_DPRINT("[FS] rootfs: lookup() finding file: %s\n", component_name);
    LLIST_FOR_EACH_ENTRY(vnode, &dir_node->childs, self) {
        if (utils_strncmp(vnode->name, component_name, utils_strlen(vnode->name)) == 0) {
            *target = vnode;
            return 0;
        }
    }
    return -1;
}

static int create(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    if (!lookup(dir_node, target, component_name)) {
        printf("[FS] rootfs: create(): file is already exist. name: %d\n", component_name);
        return -1;
    }

    FS_VNODE* new_vnode = vnode_create(component_name, S_IFREG);
    new_vnode->mount = dir_node->mount;
    new_vnode->v_ops = dir_node->v_ops;
    new_vnode->f_ops = dir_node->f_ops;
    new_vnode->parent = dir_node;

    link_list_push_back(&dir_node->childs, &new_vnode->self);
    dir_node->child_num++;

    *target = new_vnode;
    return 0;
}

static int mkdir(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    NS_DPRINT("[FS][TRACE] rootfs: mkdir(): creating %s dir\n", component_name);
    if (!lookup(dir_node, target, component_name)) {
        printf("[FS] rootfs: mkdir(): directory is already exist. name: %d\n", component_name);
        return -1;
    }

    FS_VNODE* new_vnode = vnode_create(component_name, S_IFDIR);
    new_vnode->mount = dir_node->mount;
    new_vnode->v_ops = dir_node->v_ops;
    new_vnode->f_ops = dir_node->f_ops;
    new_vnode->parent = dir_node;

    link_list_push_back(&dir_node->childs, &new_vnode->self);
    dir_node->child_num++;

    *target = new_vnode;
    NS_DPRINT("[FS][TRACE] rootfs: mkdir(): dir %s created.\n", component_name);
    return 0;

}