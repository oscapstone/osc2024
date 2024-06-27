#ifndef MY_CPIO_PARSER_H
#define MY_CPIO_PARSER_H

#define CPIO_NEWC_MAGIC "070701"
#define HEADER_SIZE  sizeof(struct cpio_newc_header)
#define MAX_ARCHIVE_SIZE (1024 * 1024 * 1024) // 1 GB

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

void cpio_ls(char *cpio_archive_start);
void cpio_cat(char *cpio_archive_start, const char *filename);
char *cpio_find(char *cpio_archive_start, const char *filename);

#endif