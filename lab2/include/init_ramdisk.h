#ifndef _INIT_RAMDISK_H
#define _INIT_RAMDISK_H 

#define CPIO_NEWC_HEADER_MAGIC  "070701"        // big endian
#define INITRAMFS_ADDR          0x20000000
	
typedef struct cpio_newc_header {
    // uses 8-byte hexadecimal fields for all numbers -> 13*8+6 = 110 bytes
    char c_magic[6];    // to identify little-endian or big-endian. -> string "070701"
    char c_ino[8];      // The inode numbers from the disk. To determine when two entries refer to the same file.
    char c_mode[8];     // specifies both the regular permissions and the file type
    char c_uid[8];      // numeric user id of the owner
    char c_gid[8];      // numeric group id of the owner
    char c_nlink[8];    // number of links to this file, directories always have a value of at least two here
    char c_mtime[8];    // last modification time of the file
    char c_filesize[8]; // size of the file
    char c_devmajor[8]; // major dev number
    char c_devminor[8]; // minor dev number
    char c_rdevmajor[8];// major dev number if file is special
    char c_rdevminor[8];// minor dev number if file is special
    char c_namesize[8]; // number of bytes in the pathname
    char c_check[8];    // always set to zero by writers and ignored by	readers.
} cpio_newc_t;
/*
The pathname is followed by NULL bytes so that the total size of the fixed header plus pathname is a multiple of four.
This format supports only 4 gigabyte files.
Hard-linked files are handled by setting the filesize to zero for each entry except the first one that appears in the archive.

ex.
cpio_newc_header -> 110 bytes
1.txt -> file1 name
This is 1.txt -> file1 content

cpio_newc_header -> 110 bytes
2.txt -> file2 name
This is 2.txt -> file2 content
TRAILER ---> END OF THE CPIO
*/

cpio_newc_t *cpio_newc_header_parser(cpio_newc_t *cur_header, char **path_name, unsigned int *file_size, char **file_content);
void init_ramdisk_ls();
void init_ramdisk_cat(const char* filename);
void init_ramdisk_callback(char *name, void *addr);

#endif