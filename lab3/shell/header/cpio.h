#ifndef CPIO_H
#define CPIO_H

#include "uart.h"
#include "utils.h"

#define CPIO_NEWC_HEADER_MAGIC "070701" // big endian

struct cpio_newc_header
{
    char c_magic[6];        // Magic number identifying the CPIO archive format. Should be "070701" for newc format.
    char c_ino[8];          // File inode number.
    char c_mode[8];         // File mode (permissions and file type).
    char c_uid[8];          // User ID of the file owner.
    char c_gid[8];          // Group ID of the file owner.
    char c_nlink[8];        // Number of hard links to the file.
    char c_mtime[8];        // Modification time of the file (timestamp).
    char c_filesize[8];     // Size of the file in bytes.
    char c_devmajor[8];     // Major number of the device (for character or block special files).
    char c_devminor[8];     // Minor number of the device.
    char c_rdevmajor[8];    // Major number of the device for special files.
    char c_rdevminor[8];    // Minor number of the device for special files.
    char c_namesize[8];     // Size of the file name including null terminator.
    char c_check[8];        // Checksum of the file header.
};
void initramfs_callback(unsigned int node_type, char *name, void *value, unsigned int name_size);
int cpio_newc_parse_header(struct cpio_newc_header *this_header_pointer, char **pathname, unsigned int *filesize, char **data, struct cpio_newc_header **next_header_pointer);
int ls();
int cat(char *path);
int execfile(char *thefilepath);
#endif