// https://github.com/libyal/dtformats/blob/main/documentation/Copy%20in%20and%20out%20(CPIO)%20archive%20format.asciidoc
#ifndef _CPIO_H
#define _CPIO_H

#include "devtree.h"

// extern char* cpio_addr; // TODO: why extern? because cpio_addr is defined in cpio.c
#define CPIO_BASE_QEMU (0x8000000)
#define CPIO_BASE_RPI  (0x20000000)


void cpio_ls();
void cpio_cat();
void initramfs_callback(char *node_name, char *prop_name, struct fdt_prop* prop);

struct cpio_newc_header {
    char c_magic[6];
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];
    char c_check[8];
};



#endif