#ifndef _CPIO_H
#define _CPIO_H

#include "device_tree.h"

#define HEADER_SIZE 110
#define QEMU_LOAD_POS ((void*)0x8000000)
#define RPI_LOAD_POS ((void*)0x20000000)
#define C_MAGIC "070701"
#define ARCHIVE_END ((char*)"TRAILER!!!")

struct cpio_newc_header {
    char	   c_magic[6]; // "070701"
    char	   c_ino[8];
    char	   c_mode[8];
    char	   c_uid[8];
    char	   c_gid[8];
    char	   c_nlink[8];
    char	   c_mtime[8];
    char	   c_filesize[8]; // 
    char	   c_devmajor[8];
    char	   c_devminor[8];
    char	   c_rdevmajor[8];
    char	   c_rdevminor[8];
    char	   c_namesize[8]; // The number	of bytes in the	pathname that follows the header // TRAILER!!!
    char	   c_check[8]; // always set to 0 and ignored
}; // 8-byte hexadecimal

void my_ls();
void my_cat();
void initramfs_callback(char *node_name, char *prop_name, struct fdt_lex_prop *prop);

#endif