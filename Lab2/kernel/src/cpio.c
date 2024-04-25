
#include "uart.h"
#include "utils.h"
#include "cpio.h"

char *cpio_addr = (char *)0x20000000;
// char *cpio_addr = (char *)0x8000000;


char *findFile(char *name)
{
    char *addr = (char *)cpio_addr;
    while (cpio_TRAILER_compare((char *)(addr+sizeof(struct new_cpio_header))) == 0)
    {
        if ((utils_string_compare((char *)(addr + sizeof(struct new_cpio_header)), name) != 0))
        {
            return addr;
        }
        struct new_cpio_header* header = (struct new_cpio_header *)addr;
        unsigned long pathname_size = utils_hex2dec(header->c_namesize,(int)sizeof(header->c_namesize));;
        unsigned long file_size =  utils_hex2dec(header->c_filesize,(int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct new_cpio_header) + pathname_size;

        utils_align(&headerPathname_size,4); 
        utils_align(&file_size,4);           
        addr += (headerPathname_size + file_size);
    }
    return 0;
}

void cpio_ls(){
	char* addr = (char*) cpio_addr;
    // uart_display_string(addr);
    // uart_display_string("\n");
    // int i = 0;
	while(1){
		uart_send_char('\r');
        uart_display_string((char *)(addr+sizeof(struct new_cpio_header)));
        uart_display_string("\n");

		struct new_cpio_header* header = (struct new_cpio_header*) addr;
		unsigned long filename_size = utils_hex2dec(header->c_namesize,(int)sizeof(header->c_namesize));
		unsigned long headerPathname_size = sizeof(struct new_cpio_header) + filename_size;
		unsigned long file_size = utils_hex2dec(header->c_filesize,(int)sizeof(header->c_filesize));
	    
		utils_align(&headerPathname_size,4);
		utils_align(&file_size,4);

        if(cpio_TRAILER_compare((char *)(addr+sizeof(struct new_cpio_header)))){
            break;
        }

		addr += (headerPathname_size + file_size);
	}
}

void cpio_cat(char *filename)
{
    char *target = findFile(filename);
    if (target)
    {
        struct new_cpio_header *header = (struct new_cpio_header *)target;
        unsigned long pathname_size = utils_hex2dec(header->c_namesize,(int)sizeof(header->c_namesize));
        unsigned long file_size = utils_hex2dec(header->c_filesize,(int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct new_cpio_header) + pathname_size;

        utils_align(&headerPathname_size,4); 
        utils_align(&file_size,4);           

        char *file_content = target + headerPathname_size;
        uart_send_char('\r');
		for (unsigned int i = 0; i < file_size; i++)
        {
            uart_send_char(file_content[i]);
        }
        uart_display_string("\n");
    }
    else
    {
        uart_display_string("\rNot found the file\n");
    }
}

int cpio_TRAILER_compare(const char* str1){
  const char* str2 = "TRAILER!!!";
  for(;*str1 !='\0'||*str2 !='\0';str1++,str2++){

    if(*str1 != *str2){
      return 0;
    }  
    else if(*str1 == '!' && *str2 =='!'){
      return 1;
    }
  }
  return 1;
}