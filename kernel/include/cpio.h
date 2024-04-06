#ifndef _CPIO_H
#define _CPIO_H

#include <int.h>

typedef struct cpio_new_ascii_header {
  char c_magic[6];  // 070701
  char c_ino[8];    // determine when two entries refer to the same file
  char c_mode[8];   // specifies both the regular permissions and the file type
  char c_uid[8];
  char c_gid[8];
  char c_nlink[8];  // number of links to this file
  char c_mtime[8];  // modification time of the file
  char c_filesize[8];
  char c_devmajor[8];
  char c_devminor[8];
  char c_rdevmajor[8];
  char c_rdevminor[8];
  char c_namesize[8];  // number of bytes in the pathname
  char c_check[8];     // always set to zero by writers and ignored by readers
} cpio_header_t;

typedef struct cpio_entry {
  char *filename;
  char *content;
} cpio_entry_t;

typedef struct cpio_stat {
  char *filename;
  size_t filename_size;

  char *content;
  size_t content_size;
} cpio_stat_t;

typedef struct file_iter_s {
  char *next_addr;
  int end;
  cpio_entry_t entry;
} file_iter_t;

void file_iter_next(file_iter_t *cur);
file_iter_t cpio_list();

cpio_stat_t parse_header(cpio_header_t *header);

#endif  // _CPIO_H
