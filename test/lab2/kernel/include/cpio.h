#ifndef CPIO_H
#define CPIO_H

#define CPIO_ADDRESS  0x8000000;

typedef struct
{
    char	   c_magic[6];
    char	   c_ino[8];
    char	   c_mode[8];
    char	   c_uid[8];
    char	   c_gid[8];
    char	   c_nlink[8];
    char	   c_mtime[8];
    char	   c_filesize[8];
    char	   c_devmajor[8];
    char	   c_devminor[8];
    char	   c_rdevmajor[8];
    char	   c_rdevminor[8];
    char	   c_namesize[8];
    char	   c_check[8];
} __attribute__((packed)) cpio_t;





int hex2int(char *p, int len);
void read(char **address, char *target, int count);
int round2four(int origin, int option);
void cpioParse(char **ramfs, char *file_name, char *file_content);
void cpioLs();
void cpioCat(char findFileName[]);
void initrd_fdt_callback(void *start, int size);
int initrdGet();

#endif