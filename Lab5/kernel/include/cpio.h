#ifndef _CPIO_H
#define _CPIO_H

#include "util.h"
#include "io.h" 
#include "type.h"
#include "devtree.h"
// #include "thread.h"
// #include "sched.h"
// #include "exception.h"

#define CPIO_END "TRAILER!!!"
#define CPIO_MAGIC "070701"
#define FIELD_SIZE 8
#define MAGIC_SIZE 6

#define CPIO_BASE_RPI 0x20000000
#define CPIO_BASE_QEMU 0x8000000

typedef struct thread thread_t;

struct cpio_newc_header {
    char c_magic[6];        // The string 070701 for new ASCII
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];     //must be 0 for FIFOs and directories
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];        //count includes terminating NUL in pathname
    char c_check[8];        // 0 for "new" portable format; for CRC format the sum of all the bytes in the file
};

void cpio_ls();
void cpio_cat();
void cpio_exec();
void initramfs_callback(char*, char*, struct fdt_prop*);
char* get_cpio_file(char*);
int get_cpio_fsize(char*);

#endif