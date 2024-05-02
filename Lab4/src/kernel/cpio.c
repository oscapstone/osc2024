#include "kernel/cpio.h"
#include "kernel/utils.h"
#include "kernel/uart.h"

void *cpio_find(char *input){
    char *temp_addr = cpio_addr;
    struct cpio_newc_header* header = (struct cpio_newc_header*)temp_addr;

    int namesize;
    int filesize;

    while(string_comp((char*)(temp_addr + sizeof(struct cpio_newc_header)), "TRAILER!!!") != 0){
        header = (struct cpio_newc_header*)temp_addr;
        namesize = h2i(header->c_namesize, 8);
        filesize = h2i(header->c_filesize, 8);

        if(!string_comp((char*)(temp_addr + sizeof(struct cpio_newc_header)), input)){
            //uart_b2x(filesize);
            //uart_putc('\n');
            if(filesize == 0){
                //uart_puts(input);
                uart_puts("Is a directory\n");
                return 0;
            }
            else{
                return (void*)((char*)(temp_addr + sizeof(struct cpio_newc_header) + namesize + align_offset((sizeof(struct cpio_newc_header) + namesize), 4) ));
            }
        }

        temp_addr += (sizeof(struct cpio_newc_header) + namesize + filesize + align_offset((sizeof(struct cpio_newc_header) + namesize), 4) + align_offset(filesize, 4));
    }
    
    //uart_puts(input);
    uart_puts("No such file or directory\n");
    return 0;
}

void cpio_ls(){
    // a temp address used for ls function
    char *temp_addr = cpio_addr;
    struct cpio_newc_header* header = (struct cpio_newc_header*)temp_addr;
    int namesize;
    int filesize; 
    // header fields are not NULL terminated, so I had to provide length in order to do string compare
    if(string_comp_l(header->c_magic, "070701", 6) != 0){
        uart_puts("cpio magic value error\n");
        return;
    }
    // loop until the end of cpio
    while(string_comp((char*)(temp_addr + sizeof(struct cpio_newc_header)), "TRAILER!!!") != 0){
        header = (struct cpio_newc_header*)temp_addr;
        namesize = h2i(header->c_namesize, 8);
        filesize = h2i(header->c_filesize, 8);

        /*uart_b2x(namesize);
        uart_putc('\n');
        uart_b2x(filesize);
        uart_putc('\n');*/

        uart_puts((char*)(temp_addr + sizeof(struct cpio_newc_header)));
        uart_putc('\n');
        //uart_puts("----------------\n");
        // followed  by NUL bytes so that the total size of the fixed header plus pathname is a multiple of four
        // the file data is padded to a multiple of four bytes.
        // (4-size%4) can get right value if size%4 != 0, so mod again to eliminate 0(if size%4 = 0, padding should be 0 instead of 4)
        temp_addr += (sizeof(struct cpio_newc_header) + namesize + filesize + align_offset((sizeof(struct cpio_newc_header) + namesize), 4) +align_offset(filesize, 4));
    }
}

void cpio_cat(char *input){
    char *temp_addr = cpio_addr;
    struct cpio_newc_header* header = (struct cpio_newc_header*)temp_addr;

    int namesize;
    int filesize;

    while(string_comp((char*)(temp_addr + sizeof(struct cpio_newc_header)), "TRAILER!!!") != 0){
        header = (struct cpio_newc_header*)temp_addr;
        namesize = h2i(header->c_namesize, 8);
        filesize = h2i(header->c_filesize, 8);

        if(!string_comp((char*)(temp_addr + sizeof(struct cpio_newc_header)), input)){
            //uart_b2x(filesize);
            //uart_putc('\n');
            if(filesize == 0){
                //uart_puts(input);
                uart_puts("Is a directory\n");
            }
            else{
                uart_puts((char*)(temp_addr + sizeof(struct cpio_newc_header) + namesize + align_offset((sizeof(struct cpio_newc_header) + namesize), 4) ));
                //uart_puts_fixed((char*)(temp_addr + sizeof(struct cpio_newc_header) + namesize + ((4 - ((sizeof(struct cpio_newc_header) + namesize)%4) ) % 4) ), filesize);
                //uart_putc('\n');
            }
            return;
        }

        temp_addr += (sizeof(struct cpio_newc_header) + namesize + filesize + align_offset((sizeof(struct cpio_newc_header) + namesize), 4) + align_offset(filesize, 4));
    }
    
    //uart_puts(input);
    uart_puts("No such file or directory\n");
}