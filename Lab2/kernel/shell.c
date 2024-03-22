#include "uart.h"

struct cpio_newc_header {
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
};

int strcmp(char *s1, char *s2) {
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (*(unsigned char *)s1) - (*(unsigned char *)s2);
}

int hex_to_int(char *p, int len) {
    int val = 0;
    int temp;
    for (int i = 0; i < len; i++) {
        temp = *(p + i);
        if (temp >= 'A') {
            temp = temp - 'A' + 10;
        } else
            temp -= '0';
        val *= 16;
        val += temp;
    }
    return val;
}

void cpio_ls(){
    struct cpio_newc_header *fs = (struct cpio_newc_header *)0x8000000;
    char *current = (char *)0x8000000;
    while (1) {
        fs = (struct cpio_newc_header *)current;
        int name_size = hex_to_int(fs->c_namesize, 8);
        int file_size = hex_to_int(fs->c_filesize, 8);
        current += 110; // size of cpio_newc_header
        if (name_size == 0) break;

        uart_puts(current);
        uart_puts("\n");
        current += name_size;
        while ((current - (char *)fs) % 4 != 0)
            current++;
        current += file_size;
        while ((current - (char *)fs) % 4 != 0)
            current++;
    }
}


void shell(char * cmd){
    if(strcmp(cmd, "help") == 0){
        uart_puts("\rhelp\t: print all available commands\n");
        uart_puts("ls\t: list all files\n");
        uart_puts("cat\t: show content of file\n");
    }
    else if(strcmp(cmd, "ls") == 0){
        cpio_ls();
        uart_puts("\r");
    }
    else{
        uart_puts("\r");
    }
}