#include "cpio.h"
#include "io/uart.h"
#include "utils/utils.h"

extern char* cpio_addr;

char* cpio_findFile(const char* name, unsigned int len) {
    char* addr = cpio_addr;
    // at the end of the cpio files has the special name TRAILER!!!
    while (utils_strncmp((char*)(addr + sizeof(struct cpio_newc_header)), "TRAILER!!!", 10) != 0) {
        struct cpio_newc_header* header = (struct cpio_newc_header*) addr;
        if (utils_strncmp(header->c_magic, "070701", 6)) {
            uart_send_string("Error CPIO type!!\n");
            uart_send_nstring(6, header->c_magic);
            uart_send_string("\n");
            return 0;
        }
        // uart_send_string("Finding: ");
        // uart_send_string((char*)(addr + sizeof(struct cpio_newc_header)));
        // uart_send_string("\n");
        if (utils_strncmp((char*)(addr + sizeof(struct cpio_newc_header)), name, len) == 0) {
            return addr;
        }
        unsigned long pathname_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));;
        unsigned long file_size =  utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_newc_header) + pathname_size;

        utils_align(&headerPathname_size, 4);
        utils_align(&file_size, 4);

        addr += (headerPathname_size + file_size);
    }

    return 0;
}

void cpio_ls() {

    char* addr = cpio_addr;

	while(utils_strncmp((char *)(addr+sizeof(struct cpio_newc_header)),"TRAILER!!!", 10) != 0) {
		

		struct cpio_newc_header* header = (struct cpio_newc_header*) addr;
        if (utils_strncmp(header->c_magic, "070701", 6)) {
            uart_send_string("Error CPIO type!!\r\n");
            uart_send_nstring(6, header->c_magic);
            return;
        }
		unsigned long filename_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
		unsigned long headerPathname_size = sizeof(struct cpio_newc_header) + filename_size;
		unsigned long file_size = utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
	    
		utils_align(&headerPathname_size,4);
		utils_align(&file_size,4);
		
		uart_send_nstring(filename_size, addr + sizeof(struct cpio_newc_header));
		uart_send_string("\n");
		
		addr += (headerPathname_size + file_size);
	}
}


void cpio_cat(char *filename, unsigned int len)
{ 
    char* addr = cpio_findFile(filename, len);
    if (!addr) {
        uart_send_string("Filename: ");
        uart_send_nstring(len, filename);
        uart_send_string(" not found\n");
        return;
    }
    struct cpio_newc_header* header = (struct cpio_newc_header*) addr;
    unsigned long filename_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
    unsigned long headerPathname_size = sizeof(struct cpio_newc_header) + filename_size;
    unsigned long file_size = utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
    
    utils_align(&headerPathname_size,4);
    utils_align(&file_size,4);
    
    char *file_content = addr + headerPathname_size;
    uart_send_string("Filename: ");
    uart_send_nstring(filename_size, addr + sizeof(struct cpio_newc_header));
    uart_send_string("\n");
    for (unsigned int i = 0; i < file_size; i++)
    {
        uart_send_char(file_content[i]);
    }
    uart_send_string("\n");
}

int cpio_get(const char *filename, unsigned int len, UPTR *content_addr, unsigned long *content_size)
{
    char* addr = cpio_findFile(filename, len);
    if (!addr) {
        return -1;
    }
    struct cpio_newc_header* header = (struct cpio_newc_header*) addr;
    unsigned long filename_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
    unsigned long headerPathname_size = sizeof(struct cpio_newc_header) + filename_size;
    unsigned long file_size = utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
    
    utils_align(&headerPathname_size,4);
    utils_align(&file_size,4);
    
    char *file_content = addr + headerPathname_size;

    *content_addr = (UPTR)file_content;
    *content_size = file_size;
    return 0;
}

