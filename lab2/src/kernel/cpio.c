#include "cpio.h"
#include "uart.h"
#include "utils.h"

char* cpio_findFile(const char* start_addr, const char* name) {

    char* addr = (char*)start_addr;

    // at the end of the cpio files has the special name TRAILER!!!
    while (string_compare((char*)(addr + sizeof(struct cpio_newc_header)), "TRAILER!!!") == 0) {
        if (string_compare((char*)(addr + sizeof(struct cpio_newc_header)), name) != 0) {
            return addr;
        }
        struct cpio_newc_header* header = (struct cpio_newc_header*) addr;
        unsigned long pathname_size = atoi(header->c_namesize,(int)sizeof(header->c_namesize));;
        unsigned long file_size =  atoi(header->c_filesize,(int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_newc_header) + pathname_size;

        align(&headerPathname_size, 4);
        align(&file_size, 4);

        addr += (headerPathname_size + file_size);
    }

    return 0;
}

void cpio_ls(const char* start_addr){
	char* addr = (char*)start_addr;

	while(string_compare((char *)(addr+sizeof(struct cpio_newc_header)),"TRAILER!!!") == 0){
		

		struct cpio_newc_header* header = (struct cpio_newc_header*) addr;
        if (string_compare(header->c_magic, "070701") == 0) {
            uart_send_string("Error CPIO type!!\r\n");
            uart_send_string_len(header->c_magic, 6);
            break;
        }
		unsigned long filename_size = atoi(header->c_namesize,(int)sizeof(header->c_namesize));
		unsigned long headerPathname_size = sizeof(struct cpio_newc_header) + filename_size;
		unsigned long file_size = atoi(header->c_filesize,(int)sizeof(header->c_filesize));
	    
		align(&headerPathname_size,4);
		align(&file_size,4);
		
		uart_send_string(addr+sizeof(struct cpio_newc_header));
		uart_send_string("\n");
		
		addr += (headerPathname_size + file_size);
	}
}


void cpio_cat(const char* start_addr, char *filename)
{
    char *target = cpio_findFile(start_addr, filename);
    if (target)
    {
        struct cpio_newc_header *header = (struct cpio_newc_header *)target;
        unsigned long pathname_size = atoi(header->c_namesize,(int)sizeof(header->c_namesize));
        unsigned long file_size = atoi(header->c_filesize,(int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_newc_header) + pathname_size;

        align(&headerPathname_size,4); 
        align(&file_size,4);           

        char *file_content = target + headerPathname_size;
		for (unsigned int i = 0; i < file_size; i++)
        {
            uart_send_char(file_content[i]);
        }
        uart_send_string("\r\n");
    }
    else
    {
        uart_send_string("File not found\n");
    }
    for(int i = 0; i < 100000; i++) {
        asm volatile("nop;");
    }
}
