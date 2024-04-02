#include "cpio.h"
#include "mini_uart.h"
#include "utils.h"

char *findFile(char *name)
{
    char *addr = (char *)cpio_addr;

    //get file name at the end of struct
    while (strcmp((char *)(addr + sizeof(struct cpio_header)), "TRAILER!!!")) //TRAILER indicating the end of the archive
    {
        if (!(strcmp((char *)(addr + sizeof(struct cpio_header)), name)))
        {
            return addr;
        }

        struct cpio_header* header = (struct cpio_header *)addr;
        unsigned long pathname_size = utils_atoi(header->c_namesize, (int)sizeof(header->c_namesize));;
        unsigned long file_size = utils_atoi(header->c_filesize, (int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_header) + pathname_size;

        utils_align(&headerPathname_size,4); 
        utils_align(&file_size,4);           
        addr += (headerPathname_size + file_size);
    }

    return 0;
}

void cpio_ls(){
	char* addr = (char*) cpio_addr;

	while (strcmp((char *)(addr+sizeof(struct cpio_header)),"TRAILER!!!")){
		struct cpio_header* header = (struct cpio_header*) addr;
		unsigned long filename_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
		unsigned long headerPathname_size = sizeof(struct cpio_header) + filename_size;
		unsigned long file_size = utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
	    
		utils_align(&headerPathname_size,4);
		utils_align(&file_size,4);
		
		uart_send_string(addr+sizeof(struct cpio_header));
		uart_send_string("\n\r");
		
		addr += (headerPathname_size + file_size);
	}

    uart_send_string("\n");
}


void cpio_cat(char *filename)
{
    char *target = findFile(filename);
    if (target)
    {
        struct cpio_header *header = (struct cpio_header *)target;
        unsigned long pathname_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
        unsigned long file_size = utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_header) + pathname_size;

        utils_align(&headerPathname_size,4); 
        utils_align(&file_size,4);           

        char *file_content = target + headerPathname_size;
		for (unsigned int i = 0; i < file_size; i++)
        {
            uart_send_char(file_content[i]);
        }
        uart_send_string("\r");
    }

    else
    {
        uart_send_string("Not found the file\n\n\r");
    }
}
