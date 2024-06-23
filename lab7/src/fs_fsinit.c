#include "fs_fsinit.h"
#include "mm.h"

void fs_early_init(void) {
    filesystem *tmpfs, *cpiofs, *uartfs;

    vfs_init();
    // init tmpfs
    tmpfs = tmpfs_init();
    uart_send_string("tmpfs_init\n");
    // init cpiofs
    cpiofs = cpiofs_init();
    // init uartfs
    uartfs = uartfs_init();

    uart_send_string("cpiofs_init\n");
    register_filesystem(tmpfs);
    uart_send_string("register tmpfs\n");
    register_filesystem(cpiofs);
    uart_send_string("register cpiofs\n");
    register_filesystem(uartfs);
    uart_send_string("register uartfs\n");

    vfs_init_rootfs(tmpfs);
    uart_send_string("init rootfs\n");
}

void fs_init(void) {
    // mount rootfs
    vfs_mkdir("/initramfs");
    uart_send_string("mkdir /initramfs\n");
    vfs_mount("/initramfs", "cpiofs");
    uart_send_string("mount /initramfs\n");

    // mount uartfs
    vfs_mkdir("/dev");
    uart_send_string("mkdir /dev\n");
    vfs_mkdir("/dev/uart");
    uart_send_string("mkdir /dev/uart\n");
    vfs_mount("/dev/uart", "uartfs");
}