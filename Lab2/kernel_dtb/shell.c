#include "uart.h"

static char *cpio_base;

void init_cpio(char *address)
{
    cpio_base = address;
}


struct cpio_newc_header {
    //file metadata
    char c_magic[6];         // Magic number identifying the CPIO format
    char c_ino[8];           // Inode number
    char c_mode[8];          // File mode and type
    char c_uid[8];           // User ID of file owner
    char c_gid[8];           // Group ID of file owner
    char c_nlink[8];         // Number of hard links
    char c_mtime[8];         // Modification time of file
    char c_filesize[8];      // Size of file (in hexadecimal)
    char c_devmajor[8];      // Major device number (for device files)
    char c_devminor[8];      // Minor device number (for device files)
    char c_rdevmajor[8];     // Major device number for the device file node referenced by the symlink
    char c_rdevminor[8];     // Minor device number for the device file node referenced by the symlink
    char c_namesize[8];      // Size of filename (in hexadecimal)
    char c_check[8];         // Checksum
};

int strcmp(char *s1, char *s2) {
    uart_puts2(s1);
    uart_puts2("\n");
    uart_send('\r');
    uart_puts2(s2);
    uart_puts2("\n");
    uart_send('\r');

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
    uart_send('\r');
    uart_puts("");
    struct cpio_newc_header *fs = (struct cpio_newc_header *)cpio_base;
    char *current = (char *)cpio_base;
    while (1) {
        fs = (struct cpio_newc_header *)current;
        int name_size = hex_to_int(fs->c_namesize, 8);
        int file_size = hex_to_int(fs->c_filesize, 8);
        current += 110; // size of cpio_newc_header
        if (strcmp(current, "TRAILER!!!") == 0)
            break;

        uart_puts(current);
        uart_puts("\n");
        uart_send('\r');
        current += name_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
        
        current += file_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
    }
}

void cpio_cat(){
    uart_send('\r');
    uart_puts("Filename: ");
    char in_char;
    char filename[100];
    int idx = 0;
    while(1){
        in_char = uart_getc();
        uart_send(in_char);
        if(in_char == '\n'){
            filename[idx] = '\0';
            idx = 0;
            break;
        }
        else{
            filename[idx] = in_char;
            idx++;
        }
    }
    
    struct cpio_newc_header *fs = (struct cpio_newc_header *)cpio_base;
    char *current = (char *)cpio_base;
    while (1) {
        fs = (struct cpio_newc_header *)current;
        int name_size = hex_to_int(fs->c_namesize, 8);
        int file_size = hex_to_int(fs->c_filesize, 8);
        current += 110; // size of cpio_newc_header
        if (strcmp(current, "TRAILER!!!") == 0){
            uart_send('\r');
            uart_puts(filename);
            uart_puts(": No such file.\n");
            break;
        }
        if (strcmp(current, filename) == 0){
            current += name_size;
            if((current - (char *)fs) % 4 != 0)
                current += (4 - (current - (char *)fs) % 4);
            uart_send('\r');
            uart_puts(current);
            uart_puts("\n");
            break;
        }
        current += name_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
        
        current += file_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
    }
}

void shell(char * cmd){
    if(strcmp(cmd, "help") == 0){
        uart_send('\r');
        uart_puts("help\t: print all available commands\n");
        uart_send('\r');
        uart_puts("ls\t: list all files\n");
        uart_send('\r');
        uart_puts("cat\t: show content of file\n");
    }
    else if(strcmp(cmd, "ls") == 0){
        cpio_ls();
    }
    else if(strcmp(cmd, "cat") == 0){
        cpio_cat();
    }
}