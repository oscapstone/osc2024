#ifndef _CPIO_H_
#define _CPIO_H_

#define CPIO_NEWC_MAGIC_NUM "070701"

// new ascii archive format
typedef struct _cpio_newc_header
{
    char c_magic[6];            // fixed, "070701".
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
} cpio_newc_header;

/* write pathname, data, next header into corresponding parameter*/
int cpio_newc_parse_header(cpio_newc_header *this_header_pointer,
        char **pathname, unsigned int *filesize, char **data,
        cpio_newc_header **next_header_pointer);

#endif