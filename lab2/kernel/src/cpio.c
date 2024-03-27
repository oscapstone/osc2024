#include "uart.h"

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

int hex2int(char *s, int n)
{
    int r = 0;
    while (n-- > 0)
    {
        r <<= 4;
        if (*s >= '0' && *s <= '9')
            r += *s++ - '0';
        else if (*s >= 'A' && *s <= 'F')
            r += *s++ - 'A' + 10;
        else if (*s >= 'a' && *s <= 'f')
            r += *s++ - 'a' + 10;
    }
    return r;
}

void cpio_list(char *buf)
{
    while(!memcmp(buf,"070701",6) && memcmp(buf+sizeof(cpio_t),"TRAILER!!!",10)) {
        cpio_t *header = (cpio_t*)buf;
        int ns=hex2int(header->c_namesize,8);
        int fs=hex2int(header->c_filesize,8);
        uart_puts(buf+sizeof(cpio_t));      // filename
        uart_puts("\n");
        // jump to the next file
        if (fs % 4 != 0)
            fs += 4 - fs % 4;
        if ((sizeof(cpio_t) + ns) % 4 != 0)
            buf += (sizeof(cpio_t) + ns + (4 - (sizeof(cpio_t) + ns) % 4) + fs);
        else
            buf += (sizeof(cpio_t) + ns + ((sizeof(cpio_t) + ns) % 4) + fs);
    }
}

void cpio_cat(char *buf, char *filename)
{
    while(!memcmp(buf,"070701",6) && memcmp(buf+sizeof(cpio_t),"TRAILER!!!",10)) {
        cpio_t *header = (cpio_t*)buf;
        int ns=hex2int(header->c_namesize,8);
        int fs=hex2int(header->c_filesize,8);
        if (!memcmp(buf+sizeof(cpio_t),filename,ns)) {
            char *filedata = buf + sizeof(cpio_t) + ns;
            while(fs--) {
                uart_send(*filedata++);
            }
            uart_puts("\n");
            return;
        }
        if (fs % 4 != 0)
            fs += 4 - fs % 4;
        if ((sizeof(cpio_t) + ns) % 4 != 0)
            buf += (sizeof(cpio_t) + ns + (4 - (sizeof(cpio_t) + ns) % 4) + fs);
        else
            buf += (sizeof(cpio_t) + ns + ((sizeof(cpio_t) + ns) % 4) + fs);
    }
    uart_puts("No such file\n");
}