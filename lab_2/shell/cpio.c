#include "include/cpio.h"
#include "include/uart.h"
#include "include/utils.h"

char* cpio_addr = (char *)0x8000000;;

void cpio_ls(){
	char* addr = (char*) cpio_addr;
	while(strcmp((char *)(addr+sizeof(struct cpio_header)),"TRAILER!!!") != 0){
		struct cpio_header* header = (struct cpio_header*) addr;
		unsigned long filename_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
		unsigned long headerPathname_size = sizeof(struct cpio_header) + filename_size;
		unsigned long file_size = utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
		utils_align(&headerPathname_size,4);
		utils_align(&file_size,4);
		uart_puts(addr+sizeof(struct cpio_header));
		uart_puts("\n");
		addr += (headerPathname_size + file_size);
	}
}

void cpio_cat(char *filename){
    char *target = findFile(filename);
    if (target){
        struct cpio_header *header = (struct cpio_header *)target;
        unsigned long pathname_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
        unsigned long file_size = utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
        //The header is followed by the pathname of the entry, then is content of file
        unsigned long headerPathname_size = sizeof(struct cpio_header) + pathname_size;

        utils_align(&headerPathname_size,4); 
        utils_align(&file_size,4);           
        char *file_content = target + headerPathname_size;
		for (unsigned int i = 0; i < file_size; i++)
        {
            uart_send(file_content[i]);
        }
        uart_puts("\n");
    }
    else
    {
        uart_puts("Not found the file\n");
    }
}

char *findFile(char *name){
    char *addr = (char *)cpio_addr;
    while (strcmp((char *)(addr + sizeof(struct cpio_header)), "TRAILER!!!") != 0){
        //The header is followed by the pathname of the entry
        if ((strcmp((char *)(addr + sizeof(struct cpio_header)), name) == 0)){
            return addr;
        }
        struct cpio_header* header = (struct cpio_header *)addr;
        unsigned long pathname_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));;
        unsigned long file_size =  utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_header) + pathname_size;

        utils_align(&headerPathname_size,4); 
        utils_align(&file_size,4);           
        addr += (headerPathname_size + file_size);
    }
    return 0;
}