extern char * cpio_addr;

void cpio_ls();
void cpio_cat(char* filename);
int cpio_TRAILER_compare(const char* str1);
	
struct new_cpio_header {
    // uses 8-byte	hexadecimal fields for all numbers
    char c_magic[6];    //determine whether this archive is written with little-endian or big-endian integers.
    char c_ino[8];      //determine when two entries refer to the same file.
    char c_mode[8];     //specifies	both the regular permissions and the file type.
    char c_uid[8];      // numeric user id
    char c_gid[8];      // numeric group id
    char c_nlink[8];    // number of links to this file.
    char c_mtime[8];    // Modification time of the file
    char c_filesize[8]; // size of the file
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8]; // number of bytes in the pathname
    char c_check[8];    // always set to zero by writers and ignored by	readers.
};

struct old_cpio_header {
    char    c_magic[6];
    char    c_dev[6];
    char    c_ino[6];
    char    c_mode[6];
    char    c_uid[6];
    char    c_gid[6];
    char    c_nlink[6];
    char    c_rdev[6];
    char    c_mtime[11];
    char    c_namesize[6];
    char    c_filesize[11];
};

// struct file {
// 	struct cpio_header* file_header;
// 	unsigned long filename_size;
// 	unsigned long headerPathname_size;
// 	unsigned long file_size;
// 	char* file_content_head;
// };