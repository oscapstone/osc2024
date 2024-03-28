#include "header/uart.h"
#include "header/cpio.h"
#include "header/utils.h"

// uint32_t DTB_CPIO_BASE = CPIO_BASE_QEMU;
// uint32_t DTB_CPIO_BASE = CPIO_BASE_RPI;
uint32_t DTB_CPIO_BASE = 0; // test for devtree

void cpio_ls() {
    struct cpio_newc_header *head = (struct cpio_newc_header *) DTB_CPIO_BASE;
    uint32_t h_size = sizeof(struct cpio_newc_header); //110 bytes
    // uart_send_string("\r\n");
    // uart_send_string("DTB CPIO BASE is ");
    // uart_hex(DTB_CPIO_BASE);
    
    while (!memcmp(head, "070701", 6) && memcmp((((void*) head) + h_size), "TRAILER!!!", 10)) {
        char *f_name = ((void*) head) + h_size;
        // if (strcmp(f_name, "TRAILER!!!")) break;
        uart_send_string("\r\n");
        uart_send_string(f_name);
        // uart_send_string("\r\n");
        // uart_send_string(f_name);
        // uart_send_string(head->c_namesize);
        // uart_send_string("\r\n");
        // uart_send_string(head->c_filesize);
        // uart_hex((uint32_t) head);

        int n_size = hex2int(head->c_namesize, 8);
        int f_size = hex2int(head->c_filesize, 8);
        
        int offset = h_size + n_size;
        if(f_size % 4 != 0) f_size += (4 - f_size % 4);
        if(offset % 4 != 0) offset += (4 - offset % 4);
        
        // uart_send_string("\r\n");
        // uart_hex(n_size);
        // uart_send_string("\r\n");
        // uart_hex(f_size);
        // uart_send_string("\r\n");
        // uart_hex(offset);

        head = ((void*) head) + offset + f_size;
        // uart_send_string("\r\n");
        // uart_hex((uint32_t) head);
    }
}

void cpio_cat() {
    struct cpio_newc_header *head = (struct cpio_newc_header *) DTB_CPIO_BASE;
    uint32_t h_size = sizeof(struct cpio_newc_header); //110 bytes
    
    char input[256];

    uart_send_string("\r\n");
    uart_send_string("Filename: ");
    read_cmd(input);
    
    int no_match = 1;
    int i = 0;
    while (!memcmp(head, "070701", 6)) {
        char *f_name = ((void*) head) + h_size;

        if (strcmp(f_name, "TRAILER!!!")) break;
        int n_size = hex2int(head->c_namesize, 8);
        int f_size = hex2int(head->c_filesize, 8);
        int offset = h_size + n_size;
        if (offset % 4 != 0) offset += (4 - offset % 4);

        if (strcmp(f_name, input)) {
            uart_send_string("\r\n");
            no_match = 0;
            char *f_content = ((void*) head) + offset;
            for (int i = 0; i < f_size; i++) {
                uart_send_char(f_content[i]);
            }
            break;
        }

        if (f_size % 4 != 0) f_size += (4 - f_size % 4);
        head = ((void*) head) + offset + f_size;
        
        i++;
        if (i == 5) break;
    }
    if (no_match) {
        uart_send_string("\r\n");
        uart_send_string("File not found");
    }
}

void initramfs_callback(char *node_name, char *prop_name, struct fdt_prop* prop) {
    
    if (strcmp(node_name, "chosen")  && strcmp(prop_name, "linux,initrd-start") ) {
        DTB_CPIO_BASE = (uint32_t)((long unsigned int) fdt32_to_cpu(*((unsigned int*)(prop + 1))));
        // uart_send_string("DTB CPIO BASE is ");
        // uart_send_char(DTB_CPIO_BASE);
        // uart_send_string("\n");
    }
    
}
