#ifndef CPIO_H
#define CPIO_H

#define CPIO_ADDRESS  0x8000000;

//https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
//header + pathname + NULL + content
typedef struct
{
    //the  endianness of 16 bit integers must be determined by observing the magic number at the start of the header
    //8-byte hexadecimal => [8]
    char	   c_magic[6];  //The string "070701" 
    char	   c_ino[8];    //The inode numbers from the disk
    char	   c_mode[8];   //specifies both the regular permissions and the file type
    char	   c_uid[8];    //The numeric user	id  of the owner
    char	   c_gid[8];    //The numeric group id	of the owner
    char	   c_nlink[8];  //number of links to this file
                            //Directories  always  have  a value of	at least two here.  Note that hardlinked files include file data with every copy in the	archive.
    char	   c_mtime[8];  //Modification time of the	file
    char	   c_filesize[8];
    char	   c_devmajor[8];   //contains	the associated device number
    char	   c_devminor[8];   //contains	the associated device number
                                //For  all  other  entry types, it should be set to zero by writers and ignored by readers.
    char	   c_rdevmajor[8];  //?
    char	   c_rdevminor[8];  //?
    char	   c_namesize[8];   //The number of bytes in the pathname
    char	   c_check[8];  //This field is always set to zero	by  writers  and  ignored  by readers.
} __attribute__((packed)) cpio_t; 
//The packed attribute specifies that a variable or structure field should have the smallest possible alignment—one byte for a variable, and one bit for a field,




int hex2int(char *p, int len);
void read(char **address, char *target, int count);
int round2four(int origin, int option);
void cpioParse(char **ramfs, char *file_name, char *file_content);
void cpioLs();
void cpioCat(char findFileName[]);
void initrd_fdt_callback(void *start, int size);
int initrdGet();

#endif