#ifndef CPIO_H
#define CPIO_H

#include "uart.h"
#include "string.h"
#include "utils.h"
#include "exec.h"

#define CPIO_NEWC_HEADER_MAGIC "070701"    // big endian

struct cpio_newc_header 
{
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

/* write pathname,data,next header into corresponding parameter*/
int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer,
        char **pathname, unsigned int *filesize, char **data,
        struct cpio_newc_header **next_header_pointer);

int  ls(char* working_dir);
int  cat(char* thefilepath);
int execfile(char *thefilepath);
#endif