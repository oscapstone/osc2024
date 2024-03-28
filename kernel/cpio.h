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

typedef struct {
    char *filename;
    char *content;
} cpio_meta_t;

typedef struct {
    void (*cat)();
    void (*stat)();
} cpio_ops_t;

// FIXME: use linked list structure for now
typedef struct cpio_obj {
    cpio_hdr_t  *hdr;
    cpio_meta_t *meta;
    cpio_ops_t  *ops;
    struct cpio_obj *next;
    // FIXME: use doubly linked list maybe?
} cpio_obj_t;


void cpio_cat();
void cpio_stat();
cpio_obj_t *init_cpio_obj();
int parse(cpio_obj_t *, char *);
int parse_c_magic(cpio_obj_t *, char *);
int parse_8_bytes(char *, char *);
int parse_filename(cpio_obj_t *, char *);
int parse_content(cpio_obj_t *, char *);

#endif
