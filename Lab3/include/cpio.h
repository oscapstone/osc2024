#ifndef CPIO_H
#define CPIO_H

extern char* cpio_addr; //from dtb.c get_cpio_addr()
int hex2int(char *hex);
int round2four(int origin, int option);
void read(char **address, char *target, int count);

//https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
//header + pathname + NULL + content	
struct cpio_header {
    //the  endianness of 16 bit integers must be determined by observing the magic number at the start of the header
    //8-byte hexadecimal => [8]
    // uses 8-byte hexadecimal fields for all numbers
    char c_magic[6];    //should be "070701" present it's new ASCII
    char c_ino[8];      //determine when two entries refer to the same file., The inode numbers from the disk
    char c_mode[8];     //specifies	both the regular permissions and the file type.
    char c_uid[8];      // numeric user id
    char c_gid[8];      // numeric group id
    char c_nlink[8];    // number of links to this file.
    char c_mtime[8];    // Modification time of the file
    char c_filesize[8]; // size of the file
    char c_devmajor[8]; //contains	the associated device number
    char c_devminor[8]; //contains	the associated device number
                        //For  all  other  entry types, it should be set to zero by writers and ignored by readers.
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8]; // number of bytes in the pathname, This count includes the trailing NUL byte.
    char c_check[8];    // always set to zero by writers and ignored by	readers.
};

void cpio_parse_header(char **address, char *file_name, char *file_content);
void cpio_ls();
void cpio_cat(char *filename);
void *cpio_find_file(char *name);
void cpio_run_executable(char executable_name[]);

#endif