

// add memory compare, gcc has a built-in for that, clang needs implementation
#ifdef __clang__
int memcmp(void *s1, void *s2, int n)
{
    unsigned char *a=s1,*b=s2;
    while(n-->0){ if(*a!=*b) { return *a-*b; } a++; b++; }
    return 0;
}
#else
#define memcmp __builtin_memcmp
#endif


/*
    https://man.freebsd.org/cgi/man.cgi?query=cpio
    https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5

    CPIO New ASCII Format
        The  "new"  ASCII format uses 8-byte hexadecimal fields for all numbers
        and separates device numbers into separate fields for major  and  minor
        numbers.

    magic   The string "070701".
    check   This field is always set to zero  by  writers  and  ignored  by readers.
*/

typedef struct {
    char    c_magic[6];         /* Magic header '070701'. */
    char    c_ino[8];           /* "i-node" number. */
    char    c_mode[8];          /* Permisions. */
    char    c_uid[8];           /* User ID. */
    char    c_gid[8];           /* Group ID. */
    char    c_nlink[8];         /* Number of hard links. */
    char    c_mtime[8];         /* Modification time. */
    char    c_filesize[8];      /* File size. */
    char    c_devmajor[8];      /* File size. */
    char    c_devminor[8];
    char    c_rdevmajor[8];
    char    c_rdevminor[8];
    char    c_namesize[8];
    char    c_check[8];
} cpio_newc_header;


/**
 * Helper function to convert ASCII octal number into binary
 * s string
 * n number of digits
 */
int oct2bin(char *s, int n)
{
    int r=0;
    while(n-->0) {
        r<<=3;
        r+=*s++-'0';
    }
    return r;
}


