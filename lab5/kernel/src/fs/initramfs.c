
#include "initramfs.h"

#include "mm/mm.h"
#include "utils/printf.h"
#include "utils/utils.h"
#include "cpio.h"

extern char* cpio_addr;

static int write(FS_FILE *file, const void *buf, size_t len);
static int read(FS_FILE *file, void *buf, size_t len);
static int open(FS_VNODE *file_node, FS_FILE **target);
static int close(FS_FILE *file);
static long lseek64(FS_FILE *file, long offset, int whence);
static int ioctl(FS_FILE *file, unsigned long request, ...);

FS_FILE_OPERATIONS initramfs_f_ops = {
    write,
    read,
    open,
    close,
    lseek64,
    ioctl
};

static int lookup(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
static int create(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
static int mkdir(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
FS_VNODE_OPERATIONS initramfs_v_ops = {
    lookup,
    create,
    mkdir
};

static void init_cpio_files(FS_MOUNT* mount) {
    if (mount == NULL) {
        printf("[FS][ERROR] initramfs: mount structure is NULL. can not initialize.\n");
        return;
    }
    FS_VNODE* dir = mount->root;
    FS_VNODE* target = NULL;

    char* addr = cpio_addr;
    // at the end of the cpio files has the special name TRAILER!!!
    while (utils_strncmp((char*)(addr + sizeof(struct cpio_newc_header)), "TRAILER!!!", 10) != 0) {
        struct cpio_newc_header* header = (struct cpio_newc_header*) addr;
        if (utils_strncmp(header->c_magic, "070701", 6)) {
            return;
        }

        unsigned long pathname_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));;
        unsigned long file_size =  utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_newc_header) + pathname_size;

        const char* name = (const char*)(addr + sizeof(struct cpio_newc_header));

        utils_align(&headerPathname_size, 4);
        utils_align(&file_size, 4);

        if (lookup(dir, &target, name) == 0) {
            printf("[FS][WARN] initramfs: init_cpio_files() file already exist. file: %s\n", name);
        } else {
            target = vnode_create(name, S_IFREG);
            target->mount = dir->mount;
            target->v_ops = dir->v_ops;
            target->f_ops = dir->f_ops;
            target->parent = dir;

            link_list_push_back(&dir->childs, &target->self);
            dir->child_num++;

            target->content = (void*) ((U64)addr + headerPathname_size);
            target->content_size = file_size;
            NS_DPRINT("[FS][TRACE] CPIO file: %s added.\n", target->name);
        }


        addr += (headerPathname_size + file_size);
    }


}

static int setup_mount(FS_FILE_SYSTEM* fs, FS_MOUNT* mount) {
    NS_DPRINT("[FS] initramfs mounting...\n");
    NS_DPRINT("[FS] initramfs: mounting on %s\n", mount->root->name);
    mount->root->mount = mount;
    mount->root->f_ops = &initramfs_f_ops;
    mount->root->v_ops = &initramfs_v_ops;
    mount->fs = fs;

    // initialize the CPIO files
    init_cpio_files(mount);

    NS_DPRINT("[FS] initramfs mounted\n");
    return 0;
}


FS_FILE_SYSTEM* initramfs_create() {
    FS_FILE_SYSTEM* fs = kmalloc(sizeof(FS_FILE_SYSTEM));
    fs->name = "initramfs";
    fs->setup_mount = &setup_mount;
    link_list_init(&fs->list);
    return fs;
}

static int write(FS_FILE *file, const void *buf, size_t len) {
    // can't write, the initramfs is read only
    return -1;
}

static int read(FS_FILE *file, void *buf, size_t len) {
    NS_DPRINT("[FS] initramfs: read() reading file: %s\n", file->vnode->name);
    FS_VNODE* vnode = file->vnode;
    if (!S_ISREG(vnode->mode)) {
        printf("[FS] read(): not a regular file\n");
        return -1;
    }

    int min = (len > vnode->content_size - file->pos - 1) ? vnode->content_size - file->pos - 1 : len;
    if (min == 0) {
        return -1;
    }
    memcpy((void*)((char*)vnode->content + file->pos), buf, min);
    file->pos += min;
    NS_DPRINT("[FS] initramfs: read() read %d bytes\n", min);
    return min;
}

static int open(FS_VNODE *file_node, FS_FILE **target) {

    return 0;
}

static int close(FS_FILE *file) {

    return 0;
}

static long lseek64(FS_FILE *file, long offset, int whence) {
    switch (whence)
    {
    case SEEK_SET:
    {
        file->pos = offset;
        return offset;
    }    
        break;
    default:
        return -1;
        break;
    }
}

static int lookup(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    FS_VNODE* vnode = NULL;
    NS_DPRINT("[FS] initramfs: lookup() finding name: %s\n", component_name);
    LLIST_FOR_EACH_ENTRY(vnode, &dir_node->childs, self) {
        //NS_DPRINT("name name: %s, length: %d\n", vnode->name, utils_strlen(vnode->name));
        if (utils_strncmp(vnode->name, component_name, utils_strlen(vnode->name)) == 0) {
            *target = vnode;
            return 0;
        }
    }
    return -1;
}

static int create(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    // cannot create in read only file system
    return -1;
}

static int mkdir(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    // cannot mkdir in read only file system
    return -1;

}

static int ioctl(FS_FILE *file, unsigned long request, ...) {
    return -1;
}