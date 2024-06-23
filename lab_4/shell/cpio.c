#include "include/cpio.h"
#include "include/uart.h"
#include "include/stdint.h"
#include "include/utils.h"

char* cpio_addr = (char *)0x8000000;

void cpio_ls(){
	char* addr = (char*) cpio_addr;
    uart_puts((char *)(addr+sizeof(struct cpio_header)));
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

void cpio_exec(char *filename){
       char *target = findFile(filename);
    if (target){
        struct cpio_header *header = (struct cpio_header *)target;
        unsigned long pathname_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
        unsigned long file_size = utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
        unsigned long headerPathname_size = sizeof(struct cpio_header) + pathname_size;

        utils_align(&headerPathname_size,4); 
        utils_align(&file_size,4);           
        char *file_content = target + headerPathname_size;

        char *program_ptr = (char *) 0x30000; // load the user program into 0x30000
        char* start_point = (char*) 0x30000;
        char* stack_pointer = (char *) (0x30000 + 0x3000); // Stack pointer for the user program, base address+0x3000
		for (unsigned int i = 0; i < file_size; i++){
            *program_ptr ++ = *file_content++;
        }
        asm volatile(
            // 0x3c0
            "mov x0, 0x3c0;\n\t"
            "msr spsr_el1, x0;\n\t"
            // start address of user program
            "mov x1, 0x30000;\n\t"
            "msr elr_el1, x1;\n\t"
            // sp for user program
            "mov x2, %0;\n\t"
            "msr sp_el0, x2;\n\t"
            "eret;"
            : // No output operands
            : "r" (start_point), "r" (stack_pointer)
            : "x0", "x1", "x2" // tell compiler x0~x2 has been used
        );
    }
    uart_puts("completed.\n");
}



void cpio_cat(char *filename){
    char *target = findFile(filename);
    if (target){
        struct cpio_header *header = (struct cpio_header *)target;
        unsigned long pathname_size = utils_atoi(header->c_namesize,(int)sizeof(header->c_namesize));
        unsigned long file_size = utils_atoi(header->c_filesize,(int)sizeof(header->c_filesize));
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