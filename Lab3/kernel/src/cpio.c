
#include <stdint.h>
#include <stddef.h>

#include "uart.h"
#include "utils.h"
#include "cpio.h"
#include "allocator.h"
#include "printf.h"
#include "utils_h.h"

#define USTACK_SIZE 0x2000

int file_num = 0;
struct file *f = NULL;

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

void cpio_load_program(char *filename)
{
    // 查找文件，返回該文件的地址，如果找到了，prog_addr 就不為空
    char *prog_addr = findFile(filename);

    // 定義變量 put_addr，將目標地址設置為 0x200000
    void *put_addr = (void *)0x200000;

    // 如果找到了文件
    if (prog_addr)
    {
        struct new_cpio_header* header = (struct new_cpio_header*) prog_addr;

        unsigned int pathname_size = utils_hex2dec(header->c_namesize,(int)sizeof(header->c_namesize));
        unsigned int file_size = utils_hex2dec(header->c_filesize,(int)sizeof(header->c_filesize));

        unsigned int headerPathname_size = sizeof(struct new_cpio_header) + pathname_size;

        utils_align(&headerPathname_size, 4);
        utils_align(&file_size, 4);

        uart_display_string("\r----------------");
        uart_display_string(prog_addr + sizeof(struct new_cpio_header));
        uart_display_string("----------------\n");

        // 將文件內容從源地址放到目標地址上
        // 將二進制內容讀出來從0x200000開始放，所以linker script裡寫的0x200000不代表檔案就真的會load入0x20000
        char *file_content = prog_addr + headerPathname_size;
        unsigned char *target = (unsigned char *)put_addr;

        uart_display_string("put the target to 0x200000\n");

        while (file_size--)
        {
            *target = *file_content;
            target++;
            file_content++;
        }
        

        asm volatile("mov x0, 0x3c0  \n");
        asm volatile("msr SPSR_EL1, x0 \n");
        asm volatile("msr ELR_EL1, %0 \n" ::"r"(put_addr));
        asm volatile("mov x0, 0x200000");
        asm volatile("msr SP_EL0, x0 \n"); 
        asm volatile("mov x0, 0x0  \n");
        asm volatile("msr SPSR_EL1, x0 \n");  
        asm volatile("eret   \n");

        // 在這段程式碼中，關鍵字 volatile 的作用是告訴編譯器不要對這些指令進行優化或重排，
        // 以確保它們按照指定的順序執行。
        // 這是因為這些指令是直接操作 CPU 寄存器和系統狀態的，如果出現任何錯誤或異常行為，
        // 可能會導致系統崩潰或不正常運行。
    }
    else{
        // 如果沒有找到該文件，使用串口打印信息
        uart_display_string("Not found the program\n");
    }
}