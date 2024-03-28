#include "header/cpio.h"
#include "header/uart.h"
#include "header/utils.h"

// Each directory and file is recorded as a header followed by its pathname and content.
char *findFile(char *name){
    char *addr = (char *)cpio_addr;
    while (utils_string_compare((char *)(addr + sizeof(struct cpio_header)), "TRAILER!!!") == 0){
    	// 如果找到name(1 != 0), 則return
        // else 往下找
        if ((utils_string_compare((char *)(addr + sizeof(struct cpio_header)), name) != 0))
        {
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

void cpio_ls(){
	// addr指向cpio_addr的初始
	char* addr = (char*) cpio_addr;
	// 如果cpio走到底代表與TRAILER相同(1 != 0)， 代表沒檔案了所以跳出while
    	// 如果不同則繼續往下執行(0==0)
	while(utils_string_compare((char *)(addr+sizeof(struct cpio_header)),"TRAILER!!!") == 0){
		// 表示將`addr`位址處的記憶體視為`struct cpio_header`類型的資料結構
    		// 目的是為了存取 CPIO 檔案中的檔案header資訊, header就是ramdisk的起始位置
		struct cpio_header* header = (struct cpio_header*) addr;
		// `header->c_namesize`是一個filename的bytes數(char array), ex: 7B
		// (int)sizeof(header->c_namesize)`, ex:2
		// atoi輸出filename的長度
		unsigned long filename_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
		// headerPathname : /home/osc/lab2/file1.txt
		// cpio_header : /home/osc/lab2/
		// fiename_size : size(file1.txt)
		unsigned long headerPathname_size = sizeof(struct cpio_header) + filename_size;
		unsigned long file_size = utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
	   	// spec:pathname is a multiple of four. Likewise, the filedata is padded to a multiple of four bytes.
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
		for (unsigned int i = 0; i < file_size; i++)
        {
            uart_send_char(file_content[i]);
        }
        uart_send_string("\n");
    }
    else
    {
        uart_send_string("Not found the file\n");
    }
}
