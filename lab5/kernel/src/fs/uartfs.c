
#include "uartfs.h"
#include "io/uart.h"
#include "utils/printf.h"


static int write(FS_FILE *file, const void *buf, size_t len);
static int read(FS_FILE *file, void *buf, size_t len);
static int open(FS_VNODE *file_node, FS_FILE **target);
static int close(FS_FILE *file);
static long lseek64(FS_FILE *file, long offset, int whence);

FS_FILE_OPERATIONS uartfs_f_ops = {
    write,
    read,
    open,
    close,
    lseek64
};

static int lookup(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
static int create(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
static int mkdir(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name);
FS_VNODE_OPERATIONS uartfs_v_ops = {
    lookup,
    create,
    mkdir
};

static int setup_mount(FS_FILE_SYSTEM* fs, FS_MOUNT* mount) {
    NS_DPRINT("[FS] uartfs mounting...\n");
    NS_DPRINT("[FS] uartfs: mounting on %s\n", mount->root->name);
    mount->root->mount = mount;
    mount->root->f_ops = &uartfs_f_ops;
    mount->root->v_ops = &uartfs_v_ops;
    mount->fs = fs;

    // initialize the CPIO files

    NS_DPRINT("[FS] uartfs mounted\n");
    return 0;
}

FS_FILE_SYSTEM* uartfs_create() {
    FS_FILE_SYSTEM* fs = kmalloc(sizeof(FS_FILE_SYSTEM));
    fs->name = UART_FS_NAME;
    fs->setup_mount = &setup_mount;
    link_list_init(&fs->list);
    return fs;
}

static int write(FS_FILE *file, const void *buf, size_t len) {
    
    for (size_t i = 0; i < len; i++)  {
        uart_async_write_char(((U8*)buf)[i]);
    }
    uart_set_transmit_int();

    return 0;
}

static int read(FS_FILE* file, void* buf, size_t len) {

    for (size_t i = 0; i < len; i++) {
        while(uart_async_empty()) {
            asm volatile("nop");
        }
        ((char*)buf)[i] = uart_async_get_char();
    }
    return 0;
}

static int open(FS_VNODE *file_node, FS_FILE **target) {
    return 0;
}

static int close(FS_FILE *file) {
    return 0;
}

static long lseek64(FS_FILE *file, long offset, int whence) {
    return 0;
}

static int lookup(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {

    return -1;
}

static int create(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    // cannot create in uart
    return -1;
}

static int mkdir(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    // cannot mkdir in uart
    return -1;

}

