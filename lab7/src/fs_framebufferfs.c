#include "fs_framebufferfs.h"
#include "fs_tmpfs.h"
#include "exception.h"
#include "mm.h"

uint32_t __attribute__((aligned(0x10))) mbox[36];

filesystem *framebufferfs_init(void) {
    return &static_framebufferfs;
};

filesystem static_framebufferfs = {
    .name = "framebufferfs",
    .mount = framebufferfs_mount,
};

vnode_operations framebufferfs_v_ops = {
    .lookup = framebufferfs_lookup,
    .create = framebufferfs_create,
    .mkdir = framebufferfs_mkdir,
    .isdir = framebufferfs_isdir,
    .getname = framebufferfs_getname,
    .getsize = framebufferfs_getsize,
};

file_operations framebufferfs_f_ops = {
    .write = framebufferfs_write,
    .read = framebufferfs_read,
    .open = framebufferfs_open,
    .close = framebufferfs_close,
    .lseek64 = framebufferfs_lseek64,
    .ioctl = framebufferfs_ioctl
};

int framebufferfs_mount(filesystem *fs, mount *mnt) {
    uart_send_string("framebufferfs_mount\n");
    vnode *cur_node;
    framebufferfs_internal *internal;
    const char* name;
    internal = (framebufferfs_internal *)kmalloc(sizeof(framebufferfs_internal));

    cur_node = mnt -> root;
    cur_node -> v_ops -> getname(cur_node, &name);

    uart_send_string("framebufferfs_mount: name = ");
    uart_send_string(name);
    uart_send_string("\n");

    internal -> name = name;
    internal -> oldnode.mount = cur_node -> mount;
    internal -> oldnode.v_ops = cur_node -> v_ops;
    internal -> oldnode.f_ops = cur_node -> f_ops;
    internal -> oldnode.parent = cur_node -> parent;
    internal -> oldnode.internal = cur_node -> internal;

    internal -> lfb = 0;
    internal -> isopened = 0;
    internal -> isinit = 0;

    cur_node -> mount = mnt;
    cur_node -> v_ops = &framebufferfs_v_ops;
    cur_node -> f_ops = &framebufferfs_f_ops;
    cur_node -> internal = internal;
    

    return 0;
}


int framebufferfs_lookup(vnode *dir_node, vnode **target, const char *component_name) {
    return -1;
}

int framebufferfs_create(vnode *dir_node, vnode **target, const char *component_name) {
    return -1;
}

int framebufferfs_mkdir(vnode *dir_node, vnode **target, const char *component_name) {
    return -1;
}

int framebufferfs_isdir(vnode *dir_node) {
    return 0;
}

int framebufferfs_getname(vnode *dir_node, const char **name) {
    framebufferfs_internal *internal = (framebufferfs_internal *)dir_node -> internal;

    *name = internal -> name;
    return 0;
}

int framebufferfs_getsize(vnode *dir_node) {
    return -1;
}


int framebufferfs_open(vnode *file_node, file *target) {
    framebufferfs_internal *internal;
    el1_interrupt_disable();
    internal = (framebufferfs_internal *)file_node -> internal;
    
    if(internal -> isopened) {
        uart_send_string("framebufferfs_open: File already opened\n");
        el1_interrupt_enable();
        return -1;
    }

    target -> vnode = file_node;
    target -> f_pos = 0;
    target -> f_ops = file_node -> f_ops;
    uart_send_string("--------------------------------\n");

    uart_send_string("framebufferfs_open: File opened\n");
    internal -> isopened = 1;
    el1_interrupt_enable();
    return 0;
}

int framebufferfs_close(file *target) {
    framebufferfs_internal *internal = (framebufferfs_internal *)target -> vnode -> internal;

    target -> vnode = NULL;
    target -> f_pos = 0;
    target -> f_ops = NULL;

    internal -> isopened = 0;

    return 0;
}

int framebufferfs_write(file *target, const void *buf, size_t len) {
    // uart_send_string("w ");
    framebufferfs_internal *internal = (framebufferfs_internal *)target -> vnode -> internal;

    if(!internal -> isinit) {
        uart_send_string("framebufferfs_write: Framebuffer not initialized\n");
        return -1;
    }

    if(target -> f_pos + len > internal -> lfbsize) {
        uart_send_string("framebufferfs_write: Write out of bounds\n");
        return -1;
    }
    memncpy((void *)(internal -> lfb + target -> f_pos), buf, len);
    target -> f_pos += len;
    // uart_send_string("WS\n");
    return len;
}

int framebufferfs_read(file *target, void *buf, size_t len) {
    return -1;
}

long framebufferfs_lseek64(file *target, long offset, int whence) {
    el1_interrupt_disable();
    int base;
    framebufferfs_internal *internal = (framebufferfs_internal *)target -> vnode -> internal;

    switch(whence) {
        case SEEK_SET:
            base = 0;
            break;
        case SEEK_CUR:
            base = target -> f_pos;
            break;
        case SEEK_END:
            base = internal -> lfbsize;
            break;
        default:
            uart_send_string("framebufferfs_lseek64: Invalid whence\n");
            el1_interrupt_enable();
            return -1;
    }

    if(base + offset > internal -> lfbsize) {
        uart_send_string("framebufferfs_lseek64: Seek out of bounds\n");
        el1_interrupt_enable();
        return -1;
    }

    target -> f_pos = base + offset;
    el1_interrupt_enable();
    return 0;
}

int framebufferfs_ioctl(struct file *file, uint64_t request, va_list args) {
    if (request != 0) {
        return -1;
    }
    fb_info *info;

    framebufferfs_internal *internal = (framebufferfs_internal *)file -> vnode -> internal;

    if(internal -> isinit) {
        return 0;
    }
    uint32_t width, height, pitch, isrgb;

    mbox[0] = 35 * 4;
    mbox[1] = MBOX_REQUEST;

    mbox[2] = 0x48003; // set phy wh
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = 1024; // FrameBufferInfo.width
    mbox[6] = 768;  // FrameBufferInfo.height

    mbox[7] = 0x48004; // set virt wh
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = 1024; // FrameBufferInfo.virtual_width
    mbox[11] = 768;  // FrameBufferInfo.virtual_height

    mbox[12] = 0x48009; // set virt offset
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0; // FrameBufferInfo.x_offset
    mbox[16] = 0; // FrameBufferInfo.y.offset

    mbox[17] = 0x48005; // set depth
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32; // FrameBufferInfo.depth

    mbox[21] = 0x48006; // set pixel order
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1; // RGB, not BGR preferably

    mbox[25] = 0x40001; // get framebuffer, gets alignment on request
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096; // FrameBufferInfo.pointer
    mbox[29] = 0;    // FrameBufferInfo.size

    mbox[30] = 0x40008; // get pitch
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0; // FrameBufferInfo.pitch

    mbox[34] = MBOX_TAG_LAST;
    uart_send_string("Calling mailbox_call\n");
    mailbox_call(MBOX_CH_PROP, mbox);
    uart_send_string("Returned from mailbox_call\n");

    if (mbox[20] == 32 && mbox[28] != 0) {
        uart_send_string("FrameBuffer initialized\n");
        mbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
        width = mbox[5];        // get actual physical width
        height = mbox[6];       // get actual physical height
        pitch = mbox[33];       // get number of bytes per line
        isrgb = mbox[24];       // get the actual channel order
        internal->lfb = (void *)((unsigned long)mbox[28]);
        internal->lfbsize = mbox[29];
    } else {
        // Unable to set screen resolution to 1024x768x32
        uart_send_string("Unable to set screen resolution to 1024x768x32\n");
        return -1;
    }

    info = va_arg(args, void*);

    info -> width = width;
    info -> height = height;
    info -> pitch = pitch;
    info -> isrgb = isrgb;

    internal -> isinit = 1;
    
    return 0;
}