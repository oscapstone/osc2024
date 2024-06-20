#include "cpio.h"
#include "mini_uart.h"
#include "str_util.h"
#include "malloc.h"

#ifndef QEMU
void *DTB_LOAD_POS;
#endif

#ifdef QEMU
void *DTB_LOAD_POS = QEMU_LOAD_POS;
#endif

#ifndef QEMU
void initramfs_callback(char *node_name, char *prop_name, struct fdt_lex_prop *prop) {
    if (str_cmp(node_name, "chosen") == 0 && str_cmp(prop_name, "linux,initrd-start") == 0) {
        DTB_LOAD_POS = (struct cpio_newc_header*)swap_endian(*((unsigned int *)(prop + 1)));
    }
}
#endif

void my_ls() {
    struct cpio_newc_header * buf = DTB_LOAD_POS;
    char *filename;
    int namesize, filesize;
    int blocksize;
    while(1) {
        filename = (char*)buf + HEADER_SIZE;
        if(str_cmp(filename, ARCHIVE_END) == 0) { break; }
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
        if(str_cmp(filename, ARCHIVE_END) == 0) { break; }

        filesize = hexstr_to_int(buf->c_filesize);
        namesize = hexstr_to_int(buf->c_namesize);

        blocksize = HEADER_SIZE + namesize;
        if (blocksize % 4 != 0) { blocksize += 4 - (blocksize % 4); }

        if(str_cmp(filename, fname_i) == 0) {
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

void* load_usr_prog (char *progname) {
    struct cpio_newc_header * buf = DTB_LOAD_POS;
    char *filename;
    int namesize, filesize;
    int blocksize;
    char *progbase;
    char *filedata;
    
    while(1) {
        filename = (char*)buf + HEADER_SIZE;
        if(str_cmp(filename, ARCHIVE_END) == 0) {
            uart_send_string("target not found\r\n");
            break; 
        }

        filesize = hexstr_to_int(buf->c_filesize);
        namesize = hexstr_to_int(buf->c_namesize);

        blocksize = HEADER_SIZE + namesize;
        if (blocksize % 4 != 0) { blocksize += 4 - (blocksize % 4); }

        if(str_cmp(filename, progname) == 0) {
            uart_send_string("target found\r\n");
            progbase = my_malloc(filesize);
            filedata = (char*)buf + blocksize;
            for(int i = 0; i < filesize; i++) {
                progbase[i] = filedata[i];
            }

            return (void*) progbase;
        }

        if (filesize % 4 != 0) { filesize += 4 - (filesize % 4); }
        blocksize += filesize;

        buf = (void*)buf + blocksize;
    }

    return (void*) 0;
}