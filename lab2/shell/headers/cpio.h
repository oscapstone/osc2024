#ifndef __CPIO_H
#define __CPIO_H

// for qemu
// #define CPIO_ADDR (volatile unsigned int) 0x8000000

// for rpi
// #define CPIO_ADDR (volatile unsigned int) 0x20000000

#define END_TRAIL "TRAILER!!!"

extern char* cpio_addr;

struct cpio_header
{
    char c_magic[6]; //"070701"
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

struct file
{
    struct cpio_header* header;
    char *pathname;
    char *content;
};

void cpio_ls(void);
void cpio_cat(char*);

#endif