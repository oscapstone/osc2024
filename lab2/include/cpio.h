#ifndef _CPIO_H
#define _CPIO_H

struct cpio_newc_header { // 110 bytes
    char c_magic[6];
    char c_ino[8];  // inode number
    char c_mode[8]; // permissions
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8]; // number of hard links
    char c_mtime[8]; // modification time
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8]; // size of filename in bytes
    char c_check[8];    // checksum
};

static unsigned long align_up(unsigned long n, unsigned long align);
int cpio_parse_header(struct cpio_newc_header *archive, char **filename, unsigned int *_filesize, void **data,
                      struct cpio_newc_header **next);
void cpio_ls();
void cpio_cat(char *file);

#endif