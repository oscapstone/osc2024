#include "header/cpio.h"
#include "header/uart.h"
#include "header/utils.h"

char *findFile(char *name)
{
    char *addr = (char *)cpio_addr;
    while (utils_string_compare((char *)(addr + sizeof(struct cpio_header)), "TRAILER!!!") == 0)
    {
        if ((utils_string_compare((char *)(addr + sizeof(struct cpio_header)), name) != 0))
        {
            //find the file
            return addr;
        }

        //get next file
        struct cpio_header* header = (struct cpio_header *)addr;
        unsigned long pathname_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
        unsigned long file_size =  utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_header) + pathname_size;

        utils_align(&headerPathname_size,4); 
        utils_align(&file_size,4);

        addr += (headerPathname_size + file_size);
    }
    return 0;
}

void cpio_ls(){
	char* addr = (char*) cpio_addr;

    //The end of the archive is indicated by a special record with the pathname	"TRAILER!!!".
	while(utils_string_compare((char *)(addr+sizeof(struct cpio_header)),"TRAILER!!!") == 0){
		
		struct cpio_header* header = (struct cpio_header*) addr;
        //get int pathname size
		unsigned long pathname_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
        //header + pathname size
		unsigned long headerPathname_size = sizeof(struct cpio_header) + pathname_size;
        //get size of file
		unsigned long file_size = utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
	    
        //align to 4-byte
        //https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
        //The  pathname is followed by NUL bytes so that the total size of the fixed header plus pathname is a multiple of four.
		utils_align(&headerPathname_size,4);
		utils_align(&file_size,4);
		
		uart_send_string(addr+sizeof(struct cpio_header));
		uart_send_string("\n");
		
		addr += (headerPathname_size + file_size);
	}
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
        uart_send_char('\r');
		for (unsigned int i = 0; i < file_size; i++)
        {
            if(file_content[i] == '\0'){
                uart_send_char('\n');
            }
            else{
                uart_send_char(file_content[i]);
            }
        }
        uart_send_char('\r');
    }
    else
    {
        uart_send_string("Not found the file\n");
    }
}
