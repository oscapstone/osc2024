#ifndef CPIO_H
#define CPIO_H

/*
    https://man.freebsd.org/cgi/man.cgi?query=cpio
    https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5

    CPIO New ASCII Format
        The  "new"  ASCII format uses 8-byte hexadecimal fields for all numbers
        and separates device numbers into separate fields for major  and  minor
        numbers.

    magic   The string "070701".
    check   This field is always set to zero  by  writers  and  ignored  by readers.

    The end of the archive is indicated by a special record with the pathname "TRAILER!!".


    ### The cpio file will be read like:

    000001110000111.....0000        -> 110 byte header
    ABC.txt                         -> file name
    This is ABC.txt                 -> content
    
    000001110000111.....0000        -> 110 byte header
    DEF.txt                         -> file name
    This is DEF.txt                 -> content
    
    000001110000111.....0000        -> 110 byte header
    TRAILER!!!                      ---> END OF THE CPIO
*/

typedef struct {
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
} cpio_t;

typedef cpio_t* cpio_ptr_t;

#endif