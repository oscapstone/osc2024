#include "uart.h"
#include "cpio.h"
#include "memory.h"

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

void cpio_exec(char *buf, char *filename)
{
    while(!memcmp(buf,"070701",6) && memcmp(buf+sizeof(cpio_t),"TRAILER!!!",10)) {
        cpio_t *header = (cpio_t*)buf;
        int ns=hex2int(header->c_namesize,8);
        int fs=hex2int(header->c_filesize,8);
        int offset = sizeof(cpio_t) + ns;
        if(offset % 4 != 0) offset = 4 * ((offset + 4) / 4);
        if (!memcmp(buf+sizeof(cpio_t),filename,ns)) {
            char *user_program_addr = ((void*) buf) + offset;
            char *sp = user_program_addr + STACKSIZE;
            
            //from el1 to el0
            // set spsr_el1 to 0x3c0 
            // and elr_el1 to the program’s start address.
            // elr_el1 -> program start addr
            asm volatile( "msr     elr_el1, %0" :: "r" (user_program_addr) );
            asm volatile( "mov     x20 ,  0x3c0" );
            asm volatile( "msr     spsr_el1, x20" );
            // set the user program’s stack pointer 
            // to a proper position by setting sp_el0.
            // sp_el0 -> usr prog stack addr
            asm volatile( "msr     sp_el0,  %0" :: "r" (sp) );
            asm volatile( "eret    ");
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