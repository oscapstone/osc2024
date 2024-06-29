#pragma once

// Cpio Archive File Header (New ASCII Format)
typedef struct {
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
} cpio_t;

typedef struct {
  int namesize;
  int filesize;
  int headsize;
  int datasize;
  char *pathname;
} ramfsRec;

void initramfs_ls();
void initramfs_cat(const char *target);
void initramfs_callback(void *addr, char *property);
void initramfs_run(const char *target);
