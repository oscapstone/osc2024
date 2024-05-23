#include<mini_uart.h>
#include"stdio.h"
#include"shell.h"
#include"str.h"
#include"mailbox.h"
#include"reboot.h"
#include"dtb.h"
#include"cpio.h"
#include"memalloc.h"

int cmd_help(){
	puts("help:");for(int i=4;i<32;i++)puts(" ");puts("print this help menu.\r\n");
	puts("hello:");for(int i=5;i<32;i++)puts(" ");puts("print Hello World!\r\n");
	puts("info:");for(int i=4;i<32;i++)puts(" ");puts("print device info.\r\n");
	puts("reboot:");for(int i=6;i<32;i++)puts(" ");puts("reboot the device.\r\n");
    puts("getsp:");for(int i=5;i<32;i++)puts(" ");puts("test stack position.\r\n");
    puts("getdtb:");for(int i=6;i<32;i++)puts(" ");puts("output drb address.\r\n");
    puts("fdt_cpio:");for(int i=8;i<32;i++)puts(" ");puts("call fdt.\r\n");
    puts("ls:");for(int i=2;i<32;i++)puts(" ");puts("call cpio_ls.\r\n");
    puts("cat:");for(int i=3;i<32;i++)puts(" ");puts("call cpio_cat.\r\n");
    puts("malloc:");for(int i=6;i<32;i++)puts(" ");puts("test simple_malloc.\r\n");
    
	
	return 0;
}

int cmd_hello(){
	puts("Hello World!\r\n");
	return 0;
}

int cmd_info()
{
    // print hw revision
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_BOARD_REVISION;
    pt[3] = 4;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        puts("Hardware Revision\t: 0x");
        // put_hex(pt[6]);
        put_hex(pt[5]);
        puts("\r\n");
    }
    // print arm memory
    pt[0] = 8 * 4;
    pt[1] = MBOX_REQUEST_PROCESS;
    pt[2] = MBOX_TAG_GET_ARM_MEMORY;
    pt[3] = 8;
    pt[4] = MBOX_TAG_REQUEST_CODE;
    pt[5] = 0;
    pt[6] = 0;
    pt[7] = MBOX_TAG_LAST_BYTE;

    if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
    {
        puts("ARM Memory Base Address\t: 0x");
        put_hex(pt[5]);
        puts("\r\n");
        puts("ARM Memory Size\t\t: 0x");
        put_hex(pt[6]);
        puts("\r\n");
    }
    return 0;
}

int cmd_reboot(){
    reset(0x160);
    puts("if you want to cancel reset,insert y:");
    char c;
    c=getchar();
    puts("\r\n");
    if(c == 'y' || c == 'Y'){
    	cancel_reset();
    	return 0;
    }
    while(1);
    return 0;
}

int cmd_get_sp(){
    unsigned int value=5;
    puts("0x");
    put_long_hex((unsigned long long)&value);
    puts("\r\n");
    put_long_hex((unsigned long long)get_sp());
    puts("\r\n");
    return 0;
}

int cmd_get_dtb_addr(){
    puts("dtb:address:0x");
	put_long_hex((unsigned long long)_dtb_addr);
	puts("\r\n");
    return 0;
}

int cmd_fdt_traverse(){
    fdt_traverse(initramfs_callback);
    return 0;
}

int cmd_ls(){
    cpio_ls();
    return 0;
}

int cmd_cat(){
    char filename[FILENAME_MAX_LEN];
    flush_buffer(filename,FILENAME_MAX_LEN);
    puts("file name:");
    get(filename,FILENAME_MAX_LEN);
    cpio_cat(filename);
    return 0;
}

int cmd_malloc(){
    char* ptr=simple_malloc(5);
    ptr="doge";
    char* ptr2=simple_malloc(sizeof("Thanks you,Cheems."));
    ptr2="Thanks you,Cheems.";
    puts(ptr);
    puts("\r\n");
    puts(ptr2);
    puts("\r\n");
    return 0;
}

int cmd_handler(char* cmd){
	if(!strcmp(cmd,"help"))cmd_help();
	else if(!strcmp(cmd,"hello"))cmd_hello();
	else if(!strcmp(cmd,"info"))cmd_info();
    else if(!strcmp(cmd,"reboot"))cmd_reboot();
    else if(!strcmp(cmd,"getsp"))cmd_get_sp();
    else if(!strcmp(cmd,"getdtb"))cmd_get_dtb_addr();
    else if(!strcmp(cmd,"fdt_cpio"))cmd_fdt_traverse();
    else if(!strcmp(cmd,"ls"))cmd_ls();
    else if(!strcmp(cmd,"cat"))cmd_cat();
    else if(!strcmp(cmd,"malloc"))cmd_malloc();
	else {
		puts("can't find command:");
		puts(cmd);
		puts("\r\n");
	}
	return 0;
}

void flush_buffer(char *buffer, int length)
{
    for (int i = 0; i < length; i++)
    {
        buffer[i] = '\0';
    }
    return;
};


int shell(){
    char cmd[CMD_MAX_LEN];
	while(1){
        flush_buffer(cmd,CMD_MAX_LEN);
        puts("# ");
		get(cmd,CMD_MAX_LEN);
		cmd_handler(cmd);
	}
	return 0;
}


