
#include "framebufferfs.h"
#include "io/uart.h"
#include "utils/printf.h"
#include "peripherals/mailbox.h"
#include "mm/mm.h"
#include "dev/framebuffer.h"

static int write(FS_FILE *file, const void *buf, size_t len);
static int read(FS_FILE *file, void *buf, size_t len);
static int open(FS_VNODE *file_node, FS_FILE **target);
static int close(FS_FILE *file);
static long lseek64(FS_FILE *file, long offset, int whence);
static int ioctl(FS_FILE *file, unsigned long request, ...);

FS_FILE_OPERATIONS framebufferfs_f_ops = {
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
FS_VNODE_OPERATIONS framebufferfs_v_ops = {
    lookup,
    create,
    mkdir
};

static int setup_mount(FS_FILE_SYSTEM* fs, FS_MOUNT* mount) {
    NS_DPRINT("[FS] framebufferfs mounting...\n");
    NS_DPRINT("[FS] framebufferfs: mounting on %s\n", mount->root->name);
    mount->root->mount = mount;
    mount->root->f_ops = &framebufferfs_f_ops;
    mount->root->v_ops = &framebufferfs_v_ops;
    mount->fs = fs;

    NS_DPRINT("[FS] framebufferfs mounted\n");
    return 0;
}

FS_FILE_SYSTEM* framebufferfs_create() {
    FS_FILE_SYSTEM* fs = kmalloc(sizeof(FS_FILE_SYSTEM));
    fs->name = FRAMEBUFFER_FS_NAME;
    fs->setup_mount = &setup_mount;
    link_list_init(&fs->list);
    return fs;
}

static int write(FS_FILE *file, const void *buf, size_t len) {
    memcpy(buf, (void*)(framebuffer_manager.lfb_addr + file->pos), len);
    file->pos += len;

    return len;
}

static int read(FS_FILE* file, void* buf, size_t len) {
    memcpy((void*)(framebuffer_manager.lfb_addr + file->pos), buf, len);
    file->pos += len;

    return len;
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
    {
        return -1;
    }
        break;
    }
}

static int lookup(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    return -1;
}

static int create(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    return -1;
}

static int mkdir(FS_VNODE* dir_node, FS_VNODE** target, const char* component_name) {
    return -1;

}

static int ioctl(FS_FILE *file, unsigned long request, ...) {

    va_list args;
    va_start(args, request);
    FRAMEBUFFER_INFO* info = (FRAMEBUFFER_INFO*) va_arg(args, FRAMEBUFFER_INFO*);

    // initialize the framebuffer to lab7 require
    mailbox[0] = 35 * 4;
    mailbox[1] = MBOX_REQUEST;

    mailbox[2] = 0x48003; // set phy wh
    mailbox[3] = 8;
    mailbox[4] = 8;
    mailbox[5] = 1024; // FrameBufferInfo.width
    mailbox[6] = 768;  // FrameBufferInfo.height

    mailbox[7] = 0x48004; // set virt wh
    mailbox[8] = 8;
    mailbox[9] = 8;
    mailbox[10] = 1024; // FrameBufferInfo.virtual_width
    mailbox[11] = 768;  // FrameBufferInfo.virtual_height

    mailbox[12] = 0x48009; // set virt offset
    mailbox[13] = 8;
    mailbox[14] = 8;
    mailbox[15] = 0; // FrameBufferInfo.x_offset
    mailbox[16] = 0; // FrameBufferInfo.y.offset

    mailbox[17] = 0x48005; // set depth
    mailbox[18] = 4;
    mailbox[19] = 4;
    mailbox[20] = 32; // FrameBufferInfo.depth

    mailbox[21] = 0x48006; // set pixel order
    mailbox[22] = 4;
    mailbox[23] = 4;
    mailbox[24] = 1; // RGB, not BGR preferably

    mailbox[25] = 0x40001; // get framebuffer, gets alignment on request
    mailbox[26] = 8;
    mailbox[27] = 8;
    mailbox[28] = 4096; // FrameBufferInfo.pointer
    mailbox[29] = 0;    // FrameBufferInfo.size

    mailbox[30] = 0x40008; // get pitch
    mailbox[31] = 4;
    mailbox[32] = 4;
    mailbox[33] = 0; // FrameBufferInfo.pitch

    mailbox[34] = MBOX_TAG_LAST;

    if (mailbox_call() && mailbox[20] == 32 && mailbox[28] != 0) {
        mailbox[28] &= 0x3fffffff;
        framebuffer_manager.info.width = mailbox[5];
        framebuffer_manager.info.height = mailbox[6];
        framebuffer_manager.info.pitch = mailbox[7];
        framebuffer_manager.info.isrgb = mailbox[8];
        framebuffer_manager.lfb_addr = MMU_PHYS_TO_VIRT((UPTR)mailbox[28]);
    } else {
        printf("Unable to set screen resolution to 1024x768x32\n");
        return -1;
    }

    memcpy(&framebuffer_manager.info, info, sizeof(FRAMEBUFFER_INFO));
    return 0;
}