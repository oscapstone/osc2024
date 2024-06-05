
#include "fs.h"
#include "mm/mm.h"
#include "rootfs.h"
#include "initramfs.h"
#include "utils/utils.h"
#include "utils/printf.h"

#include "uartfs.h"
#include "framebufferfs.h"

FS_MANAGER* fs_manager;

FS_FILE_SYSTEM* fs_get(const char *name);

/**
 * @param cwd
 *      current working directory
 * @param pathname
 *      path to search
 * @param parent
 *      parent directory to return
 * @param target
 *      target to return
 * @param node_name
 *      final node name to return whatever it can be find
*/
int fs_find_node(FS_VNODE* cwd, const char* pathname, FS_VNODE** parent, FS_VNODE** target, char* node_name) {

    char tmp_name[FS_MAX_NAME_SIZE];
    size_t name_offset = 0;
    FS_VNODE* current_dir = cwd;
    size_t offset = 0;

    if (pathname[offset] == '/' || current_dir == NULL) {
        current_dir = fs_get_root_node();
        offset++;
    }

    while (pathname[offset]) {

        if (utils_strncmp(&pathname[offset], "..", 2) == 0) {
            if (!current_dir->parent) {
                NS_DPRINT("[FS] fs_find_node(): error no parent!\n");
                return -1;
            }
            NS_DPRINT("[FS] fs_find_node(): current directory: %s\n", current_dir->name);
            NS_DPRINT("[FS] fs_find_node(): change to parent directory: %s\n", current_dir->parent->name);
            current_dir = current_dir->parent;
            offset += 2;
        } else if (utils_strncmp(&pathname[offset], ".", 1) == 0) {
            offset++;
        } else if (utils_strncmp(&pathname[offset], "/", 1) == 0) {
            if (name_offset == 0) {
                offset++;
                continue;
            }
            tmp_name[name_offset] = 0;      // end pointer
            FS_VNODE* next_node = NULL;
            int result = current_dir->v_ops->lookup(current_dir, &next_node, tmp_name);
            if (result != 0) {
                NS_DPRINT("[FS] fs_find_node(): file name not found. name: %s, path: %s\n", tmp_name, pathname);
                return result;
            }
            if (!S_ISDIR(next_node->mode)) {
                NS_DPRINT("[FS] fs_find_node(): file is not a directory. name: %s, path: %s\n", tmp_name, pathname);
                return -1;
            }
            current_dir = next_node;

            
            offset++;
            name_offset = 0;
        } else {
            while (pathname[offset] != '/' && pathname[offset]) {
                tmp_name[name_offset++] = pathname[offset++];
            }
        }
    }

    // is root node
    if (name_offset == 0 && current_dir == fs_get_root_node()) {
        *parent = NULL;
        *target = current_dir;
        return FS_FIND_NODE_SUCCESS;
    }

    // copy the final file name
    tmp_name[name_offset] = 0;      // end pointer
    memcpy(tmp_name, node_name, name_offset + 1);

    FS_VNODE* final_node = NULL;
    int result = current_dir->v_ops->lookup(current_dir, &final_node, tmp_name);

    *parent = current_dir;
    if (result != 0) {
        return FS_FIND_NODE_HAS_PARENT_NO_TARGET;
    }
    *target = final_node;
    NS_DPRINT("[FS] fs_find_node() file %s found. addr: 0x%p\n", final_node->name, final_node);
    return FS_FIND_NODE_SUCCESS;

    ///////////////////////////////////////////////

    // FS_VNODE* vnode = fs_manager->rootfs->root;
    // FS_VNODE* parent_ret = NULL;

    // char name[20];
    // U32 name_offset = 0;
    // U32 pathname_offset = 0;
    // if (pathname[pathname_offset++] != '/') {
    //     printf("[FS] fs_find_node(): pathname not start with root. pathname: %s\n", pathname);
    //     return -1;
    // }

    // NS_DPRINT("[FS] fs_find_node(): finding path %s\n", pathname);
    // while (pathname[pathname_offset] != 0) {
    //     name_offset = 0;
    //     while (pathname[pathname_offset] != '/' && pathname[pathname_offset] != '\0') {
    //         name[name_offset++] = pathname[pathname_offset++];
    //     }
    //     pathname_offset++;
    //     name[name_offset++] = '\0';
    //     NS_DPRINT("[FS] fs_find_node(): finding node: %s\n", name);
    //     if (pathname[pathname_offset - 1] == '\0') {
    //         memcpy(name, node_name, name_offset);       // copy the name to return
    //         NS_DPRINT("[FS] fs_find_node(): final node name %s copied.\n", node_name);
    //     }

    //     FS_VNODE* next_node;
    //     int result = vnode->v_ops->lookup(vnode, &next_node, name);
    //     if (result != 0 && pathname[pathname_offset - 1] == '\0') {
    //         *parent = vnode;
    //         NS_DPRINT("[FS] fs_find_node(): has parent but no target. parent = %s.\n", vnode->name);
    //         return FS_FIND_NODE_HAS_PARENT_NO_TARGET;
    //     }
    //     if (result != 0) {
    //         NS_DPRINT("[FS] fs_find_node(): no parent and no target.\n");
    //         return result;
    //     }
    //     parent_ret = vnode;
    //     vnode = next_node;
    //     if (pathname[pathname_offset - 1] == '\0') {
    //         break;
    //     }
    // }
    // *parent = parent_ret;
    // *target = vnode;
    // NS_DPRINT("[FS] fs_find_node(): node found %s.\n", vnode->name);
    // return FS_FIND_NODE_SUCCESS;
}

/**
 * @param pathname
 *      absolute path
*/
int vfs_lookup(FS_VNODE* cwd, const char* pathname, FS_VNODE** target) {
    FS_VNODE* parent = NULL, *child = NULL;
    char node_name[FS_MAX_NAME_SIZE];
    fs_find_node(cwd, pathname, &parent, &child, node_name);
    if (child) {
        *target = child;
        return 0;
    }
    return -1;
}

int vfs_mkdir(FS_VNODE* cwd, const char* pathname) {
    FS_VNODE* parent = NULL,* vnode = NULL;

    char node_name[FS_MAX_NAME_SIZE];
    int ret = fs_find_node(cwd, pathname, &parent, &vnode, node_name);

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
    FS_FILE_SYSTEM* fs = fs_get(ROOT_FS_NAME);
    if (!fs) {
        printf("[FS][ERROR] Failed to get root file system.\n");
        return;
    }

    fs_manager->rootfs.fs = fs;
    fs_manager->rootfs.root = vnode_create("", S_IFDIR);
    fs_manager->rootfs.fs->setup_mount(fs, &fs_manager->rootfs);

    ret = vfs_mkdir(NULL, "/initramfs"); 
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
    ret = vfs_lookup(NULL, "/initramfs", &initramfsroot);
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

    // making dev directory for devices to mounting
    ret = vfs_mkdir(NULL, "/dev"); 
    if (ret != 0) {
        printf("[FS][ERROR] Failed to make /dev directory\n");
        return;
    }

    // uart
    if (fs_register(uartfs_create())) {
        printf("[FS][ERROR] Failed to register uartfs\n");
        return;
    }
    fs = fs_get(UART_FS_NAME);
    if (!fs) {
        printf("[FS][ERROR] Failed to get uart file system.\n");
        return;
    }
    if (vfs_mkdir(NULL, "/dev/uart")) {
        printf("[FS][ERROR] Failed to make /dev/uart directory\n");
    }
    FS_VNODE* uartfsroot = NULL;
    if (vfs_lookup(NULL, "/dev/uart", &uartfsroot)) {
        printf("[FS][ERROR] Failed to get /dev/uart directory\n");
    }
    mount = kzalloc(sizeof(FS_MOUNT));
    mount->fs = fs;
    mount->root = uartfsroot;
    if (fs->setup_mount(fs, mount)) {
        printf("[FS][ERROR] Failed to mount uartfs.\n");
        return;
    }

    // framebuffferfs
    if (fs_register(framebufferfs_create())) {
        printf("[FS][ERROR] Failed to register framebufferfs\n");
        return;
    }
    fs = fs_get(FRAMEBUFFER_FS_NAME);
    if (!fs) {
        printf("[FS][ERROR] Failed to get framebuffer file system.\n");
        return;
    }
    if (vfs_mkdir(NULL, "/dev/framebuffer")) {
        printf("[FS][ERROR] Failed to make /dev/framebuffer directory\n");
    }
    FS_VNODE* framebufferfsroot = NULL;
    if (vfs_lookup(NULL, "/dev/framebuffer", &framebufferfsroot)) {
        printf("[FS][ERROR] Failed to get /dev/framebuffer directory\n");
    }
    mount = kzalloc(sizeof(FS_MOUNT));
    mount->fs = fs;
    mount->root = framebufferfsroot;
    if (fs->setup_mount(fs, mount)) {
        printf("[FS][ERROR] Failed to mount framebufferfs.\n");
        return;
    }


    NS_DPRINT("[FS][TRACE] fs_init() success.\n");
}

FS_VNODE* fs_get_root_node() {
    return fs_manager->rootfs.root;
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
int vfs_open(FS_VNODE* cwd,const char* pathname, U32 flags, FS_FILE** target) {
    NS_DPRINT("[FS] vfs_open(): opening %s, flags: %x\n", pathname, flags);
    FS_VNODE *parent = NULL, * vnode = NULL;
    char node_name[FS_MAX_NAME_SIZE];
    int ret = fs_find_node(cwd, pathname, &parent, &vnode, node_name);

    if (ret && !(flags & FS_FILE_FLAGS_CREATE)) {
        return ret;
    }

    if (vnode == NULL && ret == FS_FIND_NODE_HAS_PARENT_NO_TARGET) {
        NS_DPRINT("[FS] vfs_open(): try create file: %s\n", node_name);
        ret = parent->v_ops->create(parent, &vnode, node_name);
        if (ret) {
            return ret;
        }
        // ret will be 0
    }
    if (ret) {
        return ret;
    }



    FS_FILE* file = kzalloc(sizeof(FS_FILE));
    file->vnode = vnode;
    file->flags = flags;
    file->pos = 0;
    vnode->f_ops->open(vnode, &file);
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

long vfs_lseek64(FS_FILE* file, long offset, int whence) {
    return file->vnode->f_ops->lseek64(file, offset, whence);
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

