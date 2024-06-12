#ifndef CPIO_H
#define CPIO_H

#include "base.h"

char* cpio_findFile(const char* name, unsigned int len);
void cpio_ls();
void cpio_cat(char* filename, unsigned int len);
int cpio_get(const char *filename, unsigned int len, UPTR *content_addr, unsigned long *content_size);

struct cpio_newc_header {
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
};

#endif


