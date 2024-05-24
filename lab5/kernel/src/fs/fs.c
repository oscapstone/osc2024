
#include "fs.h"
#include "mm/mm.h"
#include "rootfs.h"
#include "initramfs.h"
#include "utils/utils.h"
#include "utils/printf.h"

FS_MANAGER* fs_manager;

FS_FILE_SYSTEM* fs_get(const char *name);

int fs_find_node(const char* pathname, FS_VNODE** parent, FS_VNODE** target, char* node_name) {
    FS_VNODE* vnode = fs_manager->rootfs->root;
    FS_VNODE* parent_ret = NULL;

    char name[20];
    U32 name_offset = 0;
    U32 pathname_offset = 0;
    if (pathname[pathname_offset++] != '/') {
        printf("[FS] fs_find_node(): pathname not start with root. pathname: %s\n", pathname);
        return -1;
    }

    NS_DPRINT("[FS] fs_find_node(): finding path %s\n", pathname);
    while (pathname[pathname_offset] != 0) {
        name_offset = 0;
        while (pathname[pathname_offset] != '/' && pathname[pathname_offset] != '\0') {
            name[name_offset++] = pathname[pathname_offset++];
        }
        pathname_offset++;
        name[name_offset++] = '\0';
        NS_DPRINT("[FS] fs_find_node(): finding node: %s\n", name);
        if (pathname[pathname_offset - 1] == '\0') {
            memcpy(name, node_name, name_offset);       // copy the name to return
            NS_DPRINT("[FS] fs_find_node(): final node name %s copied.\n", node_name);
        }

        FS_VNODE* next_node;
        int result = vnode->v_ops->lookup(vnode, &next_node, name);
        if (result != 0 && pathname[pathname_offset - 1] == '\0') {
            *parent = vnode;
            NS_DPRINT("[FS] fs_find_node(): has parent but no target. parent = %s.\n", vnode->name);
            return FS_FIND_NODE_HAS_PARENT_NO_TARGET;
        }
        if (result != 0) {
            NS_DPRINT("[FS] fs_find_node(): no parent and no target.\n");
            return result;
        }
        parent_ret = vnode;
        vnode = next_node;
        if (pathname[pathname_offset - 1] == '\0') {
            break;
        }
    }
    *parent = parent_ret;
    *target = vnode;
    NS_DPRINT("[FS] fs_find_node(): node found %s.\n", vnode->name);
    return FS_FIND_NODE_SUCCESS;
}

/**
 * @param pathname
 *      absolute path
*/
int vfs_lookup(const char* pathname, FS_VNODE** target) {
    FS_VNODE* parent = NULL, *child = NULL;
    char* node_name[20];
    fs_find_node(pathname, &parent, &child, node_name);
    if (child) {
        *target = child;
        return 0;
    }
    return -1;
}

int vfs_mkdir(const char* pathname) {
    FS_VNODE* parent = NULL,* vnode = NULL;

    char node_name[20];
    int ret = fs_find_node(pathname, &parent, &vnode, node_name);

    if (ret != FS_FIND_NODE_HAS_PARENT_NO_TARGET) {
        return ret;
    }
    NS_DPRINT("node name: %s\n", node_name);

    return parent->v_ops->mkdir(parent, &vnode, node_name);
}

void fs_init() {
    NS_DPRINT("[FS][TRACE] fs_init() started.\n");
    fs_manager = kzalloc(sizeof(FS_MANAGER));
    link_list_init(&fs_manager->filesystems);

    int ret = -1;

    // initialize root file system   

    if (fs_register(rootfs_create())) {
        printf("[FS][ERROR] Failed to register rootfs\n");
        return;
    }
    FS_FILE_SYSTEM* fs = fs_get("rootfs");
    if (!fs) {
        printf("[FS][ERROR] Failed to get root file system.\n");
        return;
    }
    fs_manager->rootfs->fs = fs;
    fs_manager->rootfs->root = vnode_create("", S_IFDIR);
    fs_manager->rootfs->fs->setup_mount(fs, fs_manager->rootfs);

    ret = vfs_mkdir("/initramfs"); 
    if (ret != 0) {
        printf("[FS][ERROR] Failed to make /initramfs directory\n");
        return;
    }
    // initialize initramfs file system
    if (fs_register(initramfs_create())) {
        printf("[FS][ERROR] Failed to register initramfs\n");
        return;
    }
    fs = fs_get("initramfs");
    if (!fs) {
        printf("[FS][ERROR] Failed to get cpio file system.\n");
        return;
    }
    FS_VNODE* initramfsroot = NULL;
    ret = vfs_lookup("/initramfs", &initramfsroot);
    if (ret != 0) {
        printf("[FS][ERROR] Failed to get /initramfs directory\n");
        return;
    }
    FS_MOUNT* mount = kzalloc(sizeof(FS_MOUNT));
    mount->fs = fs;
    mount->root = initramfsroot;

    ret = fs->setup_mount(fs, mount);
    if (ret != 0) {
        printf("[FS][ERROR] Failed to mount initramfs.\n");
        return;
    }

    NS_DPRINT("[FS][TRACE] fs_init() success.\n");
}

int fs_register(FS_FILE_SYSTEM* fs) {
    if (!fs_get(fs->name)) {
        link_list_push_back(&fs_manager->filesystems, &fs->list);
        return 0;
    }
    return -1;
}

/**
 * @param target
 *      the file struct to return
*/
int vfs_open(const char* pathname, U32 flags, FS_FILE** target) {
    FS_VNODE *parent = NULL, * vnode = NULL;
    char node_name[20];
    int ret = fs_find_node(pathname, &parent, &vnode, node_name);

    if (ret != 0 && !(flags & FS_FILE_FLAGS_CREATE)) {
        return ret;
    }

    if (vnode == NULL && ret == FS_FIND_NODE_HAS_PARENT_NO_TARGET) {
        ret = parent->v_ops->create(parent, &vnode, node_name);
        if (ret != 0) {
            return ret;
        }
        // ret will be 0
    }
    if (ret != 0) {
        return ret;
    }


    FS_FILE* file = kzalloc(sizeof(FS_FILE));
    file->vnode = vnode;
    file->flags = flags;
    file->pos = 0;
    *target = file;

    return 0;
}

int vfs_close(FS_FILE* file) {
    if (file == NULL) {
        return -1;
    }
    if (file->vnode->f_ops->close(file) != 0) {
        printf("[FS][ERROR] vfs_close(): failed to close file: %s\n", file->vnode->name);
    }
    kfree(file);
    return 0;
}

int vfs_write(FS_FILE* file, const void* buf, size_t len) {
    return file->vnode->f_ops->write(file, buf, len);
}

int vfs_read(FS_FILE* file, void* buf, size_t len) {
    return file->vnode->f_ops->read(file, buf, len);
}

FS_FILE_SYSTEM* fs_get(const char *name)
{
    FS_FILE_SYSTEM *fs;
    LLIST_FOR_EACH_ENTRY(fs, &fs_manager->filesystems, list)
    {
        if (utils_strncmp(fs->name, name, utils_strlen(name)) == 0)
        {
            return fs;
        }
    }
    return NULL;
}

FS_VNODE *vnode_create(const char *name, U32 flags)
{
    FS_VNODE *vnode = kzalloc(sizeof(FS_VNODE));
    link_list_init(&vnode->childs);
    link_list_init(&vnode->self);
    vnode->child_num = 0;
    vnode->parent = NULL;

    size_t name_len = utils_strlen(name);
    vnode->name = kzalloc(sizeof(name_len));
    memcpy(name, vnode->name, name_len);

    vnode->mode = flags;

    vnode->content = NULL;
    vnode->content_size = 0;

    return vnode;
}

