#include "uart.h"
#include "cpio.h"

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

int hex2int(char *s, int n)
{
    int r = 0;
    while (n-- > 0) {
        r <<= 4;
        if (*s >= '0' && *s <= '9') r += *s++ - '0';
        else if (*s >= 'A' && *s <= 'F') r += *s++ - 'A' + 10;
        else if (*s >= 'a' && *s <= 'f') r += *s++ - 'a' + 10;
    }
    return r;
}

void cpio_list(char *buf)
{
    while(!memcmp(buf,"070701",6) && memcmp(buf+sizeof(cpio_t),"TRAILER!!!",10)) {
        cpio_t *header = (cpio_t*)buf;
        int ns=hex2int(header->c_namesize,8);
        int fs=hex2int(header->c_filesize,8);
        // print fieles name
        uart_puts(buf+sizeof(cpio_t));
        uart_puts("\n");
        // jump to the next file
        if (fs % 4 != 0)
            fs += 4 - fs % 4;
        if ((sizeof(cpio_t) + ns) % 4 != 0)
            buf += (sizeof(cpio_t) + ns + (4 - (sizeof(cpio_t) + ns) % 4) + fs);
        else
            buf += (sizeof(cpio_t) + ns + fs);
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
        // jump to the next file
        if (fs % 4 != 0)
            fs += 4 - fs % 4;
        if ((sizeof(cpio_t) + ns) % 4 != 0)
            buf += (sizeof(cpio_t) + ns + (4 - (sizeof(cpio_t) + ns) % 4) + fs);
        else
            buf += (sizeof(cpio_t) + ns + fs);
    }
    uart_puts("No such file\n");
}