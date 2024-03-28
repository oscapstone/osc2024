#ifndef __INITRAMFS_H
#define __INITRAMFS_H

#include "m_string.h"
#include "mini_uart.h"
#include "mailbox.h"
#include "peripherals/devicetree.h"

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

typedef struct cpio_path_t {
    int namesize;
    int filesize;
    char *name;
    unsigned int mode;
    char *file;
    char *next;
} cpio_path;

#define CPIO_FILE 56
#define CPIO_DIR  52

int cpio_fdt_callback(char *nodename, char *propname, char *propvalue, unsigned int proplen);
int cpio_get_start_addr(char **addr);
char* cpio_align_filename(char* addr);
char* cpio_align_filedata(char* filename, int namesize);
char* cpio_align_nextfile(char* filedata, int datasize);
int cpio_parse(cpio_path* file);


#endif