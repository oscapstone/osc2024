#ifndef __CPIO_H__
#define __CPIO_H__

#define INITRD_ADDR (0x8000000)  // QEMU: 0x8000000, Rpi3: 0x20000000
#include "malloc.h"
#include "stdint.h"

typedef struct cpio_newc_header {
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
} cpio_newc_header;

// Compiler 會對 cpio_newc_parse_header 做 alignmenrt..
#define CPIO_NEWC_HEADER_SIZE (0x70 - 2)

void cpio_init();
void cpio_newc_parser(void* callback, char* param);
void cpio_newc_parse_header(char** cpio_ptr, cpio_newc_header** header);
void cpio_newc_show_header(cpio_newc_header* header);
void cpio_newc_parse_data(char** cpio_ptr, char** buf, uint32_t size, uint32_t offset);
void cpio_ls_callback(char* param, cpio_newc_header* header, char* file_name, uint32_t name_size, char* file_data, uint32_t data_size);
void cpio_cat_callback(char* param, cpio_newc_header* header, char* file_name, uint32_t name_size, char* file_data, uint32_t data_size);

#endif