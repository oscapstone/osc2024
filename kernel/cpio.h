#ifndef _DEF_CPIO
#define _DEF_CPIO

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
} cpio_hdr_t;

typedef struct cpio_meta {
    char *filename;
    char *content;
    int namesize;
    int filesize;
    struct cpio_meta *next;
    struct cpio_meta *prev;
} cpio_meta_t;

cpio_meta_t *init_cpio_meta();

#endif
