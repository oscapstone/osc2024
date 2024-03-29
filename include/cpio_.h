#ifndef _CPIO_H
#define _CPIO_H

extern char *cpio_addr;
typedef struct cpio_newc_header {
  char c_magic[6];  // The integer value octal 070701. Determine whether this
                    // archive is written with little-endian or big-endian
                    // integers.
  char c_ino[8];    // The  device and inode numbers from the disk. Used to
                    // determine when two entries refer to	the same file.
  char c_mode[8];   // specifies both the regular permissions and the file type.
  char c_uid[8];    // User ID of the file owner
  char c_gid[8];    // Group ID of the file owner
  char c_nlink[8];  // Number of links to the file
  char c_mtime[8];  // Modification time of the file
  char c_filesize[8];  // Size of the file
  char c_devmajor[8];
  char c_devminor[8];
  char c_rdevmajor[8];
  char c_rdevminor[8];
  char c_namesize[8];  // The number of bytes in the pathname
  char c_check[8];     // Always set	to zero	 by  writers  and  ignored  by
                       // readers
} cpio_header;

void cpio_ls();
char *findFile(const char *name);
void cpio_cat(const char *filename);
char *cpio_get_file_content_st_addr(const char *filename);
#endif