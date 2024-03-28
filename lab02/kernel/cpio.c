#include "cpio.h"
#include "utils.h"
#include "uart.h"

uint32_t DTB_CPIO_BASE = 0;


/*
A cpio archive consists of one or more concatenated member files. 
Each member file contains a header optionally followed by file contents as indicated in the header. 
The end of the archive is indicated by another header describing an (empty) file named TRAILER!!.
*/


/*
Structure
header(size) + name(size) + file(size)

*/

void cpio_ls() {
    struct cpio_newc_header *head = (struct cpio_newc_header *) DTB_CPIO_BASE;//CPIO_BASE_RPI; //DTB_CPIO_BASE
    uint32_t h_size = sizeof(struct cpio_newc_header);

    while (1) {
        char *f_name = ((void*) head) + h_size;

        

        if (strcmp(f_name, "TRAILER!!!"))break;
        

        print_string(f_name);
        print_string("\r\n");

        uint32_t n_size = atoi(head->c_namesize, 8);
        uint32_t f_size = atoi(head->c_filesize, 8);
        uint32_t offset   = h_size + n_size;
        if(offset % 4 != 0) offset = 4 * ((offset + 4) / 4);
        if(f_size % 4 != 0) f_size = 4 * ((f_size + 4) / 4);
        
        head = ((void*) head) + offset + f_size;
    }
}

void cpio_cat() {
    uint32_t h_size = sizeof(struct cpio_newc_header);
    struct cpio_newc_header *head = (struct cpio_newc_header *) DTB_CPIO_BASE;//CPIO_BASE_QEMU; //CPIO_BASE_RPI;//DTB_CPIO_BASE

    char input[BUFSIZE];

    print_string("Filename: ");
    readcmd(input, BUFSIZE);

    int no_match=1;
    while (1) {
        char *f_name = ((void*) head) + h_size;

        if (strcmp(f_name, "TRAILER!!!"))break;
        

        uint32_t n_size = atoi(head->c_namesize, 8);
        uint32_t f_size = atoi(head->c_filesize, 8);
        uint32_t offset   = h_size + n_size;

        if(offset % 4 != 0) offset = 4 * ((offset + 4) / 4);


        if (strcmp(input, f_name)) {
            no_match = 0;
            char *content = ((void*) head) + offset;
            for (int i = 0; i < f_size; i++) {
                print_char(content[i]);
            }
            if (content[f_size - 1] != '\n') print_string("\r\n");

        }

        if(f_size % 4 != 0) f_size = 4 * ((f_size + 4) / 4);
        head  = ((void*) head) + offset + f_size;
    }
    if(no_match)print_string("No matching file\r\n");
}



void initramfs_callback(char *node_name, char *prop_name, struct fdt_prop* prop) {
    // if (strcmp(node_name, "chosen")) {
    //     print_string(prop_name);
    //     print_char('\n');
        
    // }
    if (strcmp(node_name, "chosen")  && strcmp(prop_name, "linux,initrd-start") ) {
        DTB_CPIO_BASE = (uint32_t)((long unsigned int) fdt32_to_cpu(*((unsigned int*)(prop + 1))));
        print_string("DTB CPIO BASE is ");
        print_h(DTB_CPIO_BASE);
        print_char('\n');
    }
    
}