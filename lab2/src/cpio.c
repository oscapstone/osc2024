#include "cpio.h"
#include "mini_uart.h"

void *DTB_LOAD_POS;

int cpio_strcmp(char* a, char* b) {
    while(*a != '\0' && *b != '\0') {
        if (*a != *b) return 0;
        else {
            a++;
            b++;
        }
    }
    if (*a != *b) return 0;
    return 1;
}

int hexstr_to_int(char* str) {
    int val = 0; 
    for(int c = 0; c < 8; c++) {
        val *= 16;
        if(*str >= 'A') {
            val += 10 + (*str - 'A');
        } 
        else {
            val += (*str - '0');
        }
        str++;
    }
    return val;
}


void initramfs_callback(char *node_name, char *prop_name, struct fdt_lex_prop *prop) {
    if (dt_strcmp(node_name, "chosen", 7) == 1 && dt_strcmp(prop_name, "linux,initrd-start", 19) == 1) {
        DTB_LOAD_POS = (void*)swap_endian(*((unsigned int *)(prop + 1)));
    }
}

void my_ls() {
    struct cpio_newc_header * buf = DTB_LOAD_POS;
    char *filename;
    int namesize, filesize;
    int blocksize;
    while(1) {
        filename = (char*)buf + HEADER_SIZE;
        if(cpio_strcmp(filename, ARCHIVE_END)) { break; }
        uart_send_string(filename);
        uart_send_string("\r\n");
        filesize = hexstr_to_int(buf->c_filesize);
        namesize = hexstr_to_int(buf->c_namesize);

        // padding
        blocksize = HEADER_SIZE + namesize;
        if (blocksize % 4 != 0) { blocksize += 4 - (blocksize % 4); }
        if (filesize % 4 != 0) { filesize += 4 - (filesize % 4); }
        blocksize += filesize;

        buf = (void*)buf + blocksize;
    }
}

void my_cat() {
    struct cpio_newc_header * buf = DTB_LOAD_POS;
    char *filename;
    int namesize, filesize;
    int blocksize;

    char fname_i[100];
    uart_send_string("Filename: ");
    unsigned int idx = 0;
    char c = '\0';
    while (1) {
        c = uart_recv();
        if (c == '\r' || c == '\n') {
            uart_send_string("\r\n");
            
            if (idx < 100) fname_i[idx] = '\0';
            else fname_i[99] = '\0';
            
            break;
        } 
        else {
            uart_send(c);
            fname_i[idx++] = c;
        } 
    }

    char *fc;
    while(1) {
        filename = (char*)buf + HEADER_SIZE;
        if(cpio_strcmp(filename, ARCHIVE_END) == 1) { break; }

        filesize = hexstr_to_int(buf->c_filesize);
        namesize = hexstr_to_int(buf->c_namesize);

        blocksize = HEADER_SIZE + namesize;
        if (blocksize % 4 != 0) { blocksize += 4 - (blocksize % 4); }

        if(cpio_strcmp(filename, fname_i)) {
            fc = (char*)buf + blocksize;
            for(int i = 0; i < filesize; i++) {
                uart_send(*fc);
                if(*fc == '\n') { uart_send('\r'); }
                fc++;
            }
            uart_send_string("\r\n");
            break;
        }

        if (filesize % 4 != 0) { filesize += 4 - (filesize % 4); }
        blocksize += filesize;

        buf = (void*)buf + blocksize;
    }
}