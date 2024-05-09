#ifndef CPIO_H
#define CPIO_H

// cpio format : https://manpages.ubuntu.com/manpages/bionic/en/man5/cpio.5.html
#define CPIO_NEWC_MAGIC "070701"
#define CPIO_NEWC_TRAILER "TRAILER!!!"

/*
 * Struct to represent the header of a file in a New ASCII Format Cpio Archive.
 */
typedef struct cpio_newc_header {
  char c_magic[6];     // Magic number (always "070701")
  char c_ino[8];       // File inode number
  char c_mode[8];      // File mode and permissions
  char c_uid[8];       // User ID
  char c_gid[8];       // Group ID
  char c_nlink[8];     // Number of links
  char c_mtime[8];     // Modification time
  char c_filesize[8];  // Size of file data
  char c_devmajor[8];  // Major number of device
  char c_devminor[8];  // Minor number of device
  char c_rdevmajor[8]; // Major number of real device
  char c_rdevminor[8]; // Minor number of real device
  char c_namesize[8];  // Length of file name
  char c_check[8];     // Checksum (always 0)
} cpio_newc_header_t;

unsigned long cpio_hexstr_2_ulong(const char *hex, int length);
int cpio_is_directory(const char *mode);
int cpio_parse_header(cpio_newc_header_t *header, const char *filename);
char *cpio_extract_file_address(const char *filename);

#endif /* CPIO_H */
