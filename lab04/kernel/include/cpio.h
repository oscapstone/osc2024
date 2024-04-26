#ifndef __CPIO_H__
#define __CPIO_H__

#include "devicetree.h"

#ifdef QEMU 
    #define CPIO_ADDR 0x8000000
#else
    #define CPIO_ADDR 0x20000000
#endif

typedef struct cpio_newc_header {
    char    c_magic[6];
    char    c_ino[8];
    char    c_mode[8];
    char    c_uid[8];
    char    c_gid[8];
    char    c_nlink[8];
    char    c_mtime[8];
    char    c_filesize[8];
    char    c_devmajor[8];
    char    c_devminor[8];
    char    c_rdevmajor[8];
    char    c_rdevminor[8];
    char    c_namesize[8];
    char    c_check[8];
} cpio_newc_header;

void cpio_list(int argc, char **argv);
void cpio_cat(int argc, char **argv);
void cpio_exec(int argc, char **argv);

#ifndef QEMU
void initramfs_callback(char* node_name, char* property_name, fdt_prop* prop);
#endif

#endif