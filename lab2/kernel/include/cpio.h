// qemu 0x8000000 
// rpi3 0x20000000
#define cpio_address 0x8000000

#define MAX_FILE_SIZE 1024

typedef struct cpio_newc_header
{
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
}FILE_HEADER;

typedef struct file
{
    FILE_HEADER *file_header;
    char *path_name;
    char *file_content;
} FILE;

extern FILE file_arr[MAX_FILE_SIZE];
extern int file_num;

void build_file_arr();
void traverse_file();
void look_file_content(char *pathname);
