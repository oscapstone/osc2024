/* cpio hpodc format */
typedef struct cpio_newc_header {
    char    c_magic[6];         /* Magic header '070701' */
    char    c_ino[8];           /* "i-node" number */
    char    c_mode[8];          /* Permisions */
    char    c_uid[8];           /* User ID */
    char    c_gid[8];           /* Group ID */
    char    c_nlink[8];         /* Number of hard links */
    char    c_mtime[8];         /* Modification time */
    char    c_filesize[8];      /* File size */
    char    c_devmajor[8];      /* Device number major */
    char    c_devminor[8];      /* Device number minor */
    char    c_rdevmajor[8];
    char    c_rdevminor[8];
    char    c_namesize[8];      /* length of the path name */
    char    c_check[8];         /* always set to zero */
} __attribute__((packed)) cpio_t;

void cpio_list(char *buf);
void cpio_cat(char *buf, char *filename);
void cpio_exec(char *buf, char *filename);