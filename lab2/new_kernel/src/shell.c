#include "mini_uart.h"
#include "shell.h"
#include "reboot.h"
#include "cpio.h"
char input_buffer[CMD_MAX_LEN];

extern char* _dtb;
const void* CPIO_DEFAULT_PLACE = (void*)(unsigned long) 0x8000000 ;

int shell_cmd_strcmp(const char* p1, const char* p2){
    const unsigned char* s1 = (const unsigned char *) p1;
    const unsigned char* s2 = (const unsigned char *) p2;
    unsigned char c1, c2;

    do {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if ( c1 == '\0')
            return c1 - c2;
    } while ( c1 == c2);
    return c1 - c2;

}

void shell_banner(void){
    uart_puts("\n==================Now is 2024==================\n");
    uart_puts("||    Here comes the OSC 2024 Lab2 new kernel  ||\n");
    uart_puts("================================================\n");
}

void shell_cmd_read(char* buffer){
    char cursor = '\0';
    int index = 0;
    while(1){
        if (index > CMD_MAX_LEN) break;

        cursor = uart_recv();
        if (cursor == '\n'){
            uart_puts("\r\n");
            break;

        }
        if (cursor > 16 && cursor < 32) 
            continue;

        if (cursor > 127) 
            continue;
        buffer[index++] = cursor;
        uart_send(cursor);
    }

}

void shell_cmd_exe(char* buffer){

    char* parameter;
    char* cmd = buffer;

    while(1){
        if(*buffer == '\0'){
            parameter = buffer;
            break;
        }
        if (*buffer == ' '){
            *buffer = '\0';
            parameter = buffer + 1;
            break;
        }
        buffer++;
    }

    if (shell_cmd_strcmp(cmd, "hello")==0){
        uart_puts("ヽ(́◕◞౪◟◕‵)ﾉ>> ");
        shell_hello_world();
    } else if (shell_cmd_strcmp(cmd, "help") == 0)
    {   
        shell_help();
    } else if (shell_cmd_strcmp(cmd, "info") == 0)
    {
        shell_info();
    } else if (shell_cmd_strcmp(cmd, "reboot") == 0)
    {
        uart_puts("You have roughly 10 seconds to cancel reboot.\nCancel reboot with\nreboot -c\n");
        shell_reboot();
    } else if (shell_cmd_strcmp(cmd, "reboot -c") == 0)
    {
        shell_cancel_reboot();
    }else if (shell_cmd_strcmp(cmd, "ls") == 0)
    {
        shell_ls(parameter);
        // uart_puts("(｡ŏ_ŏ)ls not yet! ");
    }else if (shell_cmd_strcmp(cmd, "cat") == 0)
    {
        shell_cat(parameter);
        // uart_puts("(｡ŏ_ŏ)cat not yet! ");
    }else if (shell_cmd_strcmp(cmd, "malloc") == 0)
    {
        shell_malloc();
    }else if (shell_cmd_strcmp(cmd, "dtb") == 0)
    {
        uart_puts("(｡ŏ_ŏ)ls not yet! ");
    }else if (*cmd){
        uart_puts("(｡ŏ_ŏ) Oops! ");
        uart_puts(cmd);
        uart_puts(": command not found\r\n");
    }
    
}


void shell_clear(char * buffer , int len){
    for(int i = 0 ; i < len ; i++){
        buffer[i] = '\0';
    }
}

void shell(){
    shell_clear(input_buffer, CMD_MAX_LEN);
    uart_puts("ヽ(✿ﾟ▽ﾟ)ノ >> \t");
    shell_cmd_read(input_buffer);
    shell_cmd_exe(input_buffer);
}

void shell_hello_world(){
    uart_puts("hello world!\n");
}

void shell_help(){
    uart_puts("help     : print this help menu.\n");
    uart_puts("hello    : print hello world!\n");
    uart_puts("ls       : list files.\n");
    uart_puts("cat      : print file content.\n");
    uart_puts("clear    : clear screen.\n");
    uart_puts("reboot   : reboot raspberry pi.\n");
}

void shell_info(){
    unsigned int board_revision;
    get_board_revision(&board_revision);
    uart_puts("Board revision is : 0x");
    uart_hex(board_revision);
    uart_puts("\n");
    
    unsigned int arm_mem_base_addr;
    unsigned int arm_mem_size;

    get_arm_memory_info(&arm_mem_base_addr,&arm_mem_size);
    uart_puts("ARM memory base address in bytes : 0x");
    uart_hex(arm_mem_base_addr);
    uart_puts("\n");
    uart_puts("ARM memory size in bytes : 0x");
    uart_hex(arm_mem_size);
    uart_puts("\n");

    uart_puts("\n");
    uart_puts("This is a simple shell for raspi3.\n");
    uart_puts("type help for more information\n");
}

void shell_reboot(){
    // uart_puts("reboot not yet \n");
    reset(196608);

}

void shell_cancel_reboot(){
    cancel_reset();
    uart_puts("reboot canceled. \n");
}

void shell_ls(char* dir){
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    CPIO_DEFAULT_PLACE = (void*)(unsigned long) 0x8000000;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;
    put_int(CPIO_DEFAULT_PLACE);
    uart_puts("\n");

    while (header_ptr != 0){
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if(error){
            uart_puts("cpio parse error");
            break;
        }

        //if this is not TRAILER!!! (last of file)
        if(header_ptr!=0 ) {
        uart_puts(c_filepath);
        uart_puts("\n");
        }
    }
}

void shell_cat(char* filepath)
{
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    CPIO_DEFAULT_PLACE = (void*)(unsigned long) 0x8000000;
    struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

    while(header_ptr!=0)
    {
        int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        //if parse header error
        if(error)
        {
            uart_puts("cpio parse error");
            break;
        }

        if(shell_cmd_strcmp(c_filepath, filepath)==0)
        {
            uart_puts(c_filedata);
            break;
        }

        //if this is TRAILER!!! (last of file)
        if(header_ptr==0) {
            uart_puts("cat: ");
            uart_puts(c_filepath);
            uart_puts(" No such file or directory\n");}
    }
}

char* strcpy(char *dest, const char *src)
{
    char *d = dest;
    const char *s = src;
    while( (*d++ = *s++));
        
    return dest;
}
void shell_malloc()
{
    //test malloc
    char* test1 = malloc(0x22);
    strcpy(test1,"test malloc1");
    uart_puts(test1);
    uart_hex(test1);
    uart_puts("\n");

    char* test2 = malloc(0x10);
    strcpy(test2,"test malloc2");
    uart_puts(test2);
    uart_hex(test2);
    uart_puts("\n");

    char* test3 = malloc(0x10);
    strcpy(test3,"test malloc3");
    uart_puts(test3);
    uart_hex(test3);
    uart_puts("\n");
}
