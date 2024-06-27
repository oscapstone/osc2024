#ifndef _RAMFS_H
#define _RAMFS_H

#include <lib/stdlib.h>
#include <lib/utils.h>

typedef struct cpio_newc_header {
    char c_magic[6];  // The string 070701 for new ASCII
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];  // must be 0 for FIFOs and directories
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];  // count includes terminating NUL in pathname
    char c_check[8];  // 0 for "new" portable format; for CRC format the sum of
                      // all the bytes in the file
} cpio_t;

#define MAX_FILES 100            // 預設的最大檔案數量
#define MAX_FILENAME_LENGTH 256  // 每個檔案名稱的最大長度

typedef struct {
    char file_names[MAX_FILES][MAX_FILENAME_LENGTH];
    unsigned long file_sizes[MAX_FILES];
    int file_count;
} file_list_t;

void init_ramfs_callback(void *base_addr);
file_list_t *ramfs_get_file_list();
unsigned long ramfs_get_file_size(const char *file_name);
void ramfs_get_file_contents(const char *file_name, char *buffer);
void ramfs_execute_file(const char *file_name);

#endif
