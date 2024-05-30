#include<mini_uart.h>
#include"stdio.h"
#include"shell.h"
#include"str.h"
#include"mailbox.h"
#include"reboot.h"
#include"dtb.h"
#include"cpio.h"
#include"memalloc.h"
#include "peripherals/mini_uart.h"
#include "irq.h"

int cmd_help(){
	puts("help:");for(int i=4;i<32;i++)puts(" ");puts("print this help menu.\r\n");
	puts("hello:");for(int i=5;i<32;i++)puts(" ");puts("print Hello World!\r\n");
	puts("info:");for(int i=4;i<32;i++)puts(" ");puts("print device info.\r\n");
	puts("reboot:");for(int i=6;i<32;i++)puts(" ");puts("reboot the device.\r\n");
    //puts("getsp:");for(int i=5;i<32;i++)puts(" ");puts("test stack position.\r\n");
    puts("getdtb:");for(int i=6;i<32;i++)puts(" ");puts("output drb address.\r\n");
    puts("fdt_cpio:");for(int i=8;i<32;i++)puts(" ");puts("call fdt.\r\n");
    puts("ls:");for(int i=2;i<32;i++)puts(" ");puts("call cpio_ls.\r\n");
    puts("cat:");for(int i=3;i<32;i++)puts(" ");puts("call cpio_cat.\r\n");
    puts("malloc:");for(int i=6;i<32;i++)puts(" ");puts("test simple_malloc.\r\n");
    puts("get_el:");for(int i=6;i<32;i++)puts(" ");puts("output current exception level.\r\n");
    puts("getreg:");for(int i=6;i<32;i++)puts(" ");puts("output register value.\r\n");
    puts("loadimg:");for(int i=7;i<32;i++)puts(" ");puts("load img file to load_base and exec it.\r\n");
    puts("async:");for(int i=5;i<32;i++)puts(" ");puts("test asynchronous I/O.\r\n");
    //puts("reverse:");for(int i=7;i<32;i++)puts(" ");puts("output memory reverse block\r\n");
    puts("test:");for(int i=4;i<32;i++)puts(" ");puts("run test funtion.\r\n");
    puts("buddy:");for(int i=5;i<32;i++)puts(" ");puts("init buddy system.\r\n");
    
	
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
    reset(0x400);
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

int cmd_loadimg(){
    char filename[FILENAME_MAX_LEN];
    flush_buffer(filename,FILENAME_MAX_LEN);
    puts("img name:");
    get(filename,FILENAME_MAX_LEN);
    char* load_base=mem_alin(&_stack_top,0x10000);
    //load_base=0x60000;
    puts("load base:");
    put_long_hex(load_base);
    puts("\r\n");
    if(cpio_load(filename,load_base)){
        puts("error:load imgfile false\r\n");
        return 1;
    }
    puts(load_base);
    unsigned long spsr_el1 = 0x3c0;
    unsigned long sp =((unsigned long)load_base)+0x20000;
	asm volatile ("msr spsr_el1, %0" ::"r"(spsr_el1));
	asm volatile("msr elr_el1,%0" ::"r"(load_base));
	asm volatile("msr sp_el0,%0" :: "r"(sp));
	asm volatile("eret");
    // asm volatile(
    //     "mov x30, 0xA0000;"
    //     "ret;"
    // );
    return 0;
}

int cmd_malloc(){
    char* ptr=simple_malloc(5);
    char* ptr2=simple_malloc(sizeof("Thanks you,Cheems."));
    if(ptr==NULL || ptr2==NULL){
        puts("error:memory allocate fault\r\n");
        return 0;
    }
    ptr="doge";
    
    ptr2="Thanks you,Cheems.";
    puts(ptr);
    puts("\r\n");
    puts(ptr2);
    puts("\r\n");
    return 0;
}

int cmd_output_reverse(){
    fdt_output_reverse();

    return 0;
}

int cmd_get_el(){
    int el=get_el();
    puts("current exception levels:");
    put_int(el);
    puts("\r\n");

    return 0;
}

int cmd_get_reg(){
    int regname_len=32;
    char regname[regname_len];
    unsigned long long regvalue;
    puts("register name:");
    get(regname,regname_len);
    if(!strcmp(regname,"sp")){
        
        __asm__("mov %0, sp" : "=r"(regvalue));
        puts("stack pointer:0x");
        put_long_hex(regvalue);
        puts("\r\n");
    }
    else if(!strcmp(regname,"SPSel")){
        __asm__ volatile(
            "mrs x0, SPSel;"
            "mov %0, x0" : "=r"(regvalue)
        );
        puts("SPSel:0x");
        put_long_hex(regvalue);
        puts("\r\n");
    }
    else if(!strcmp(regname,"timer")){
        __asm__ volatile(
            "mrs x0, cntpct_el0;"
            "mov %0, x0" : "=r"(regvalue)
        );
        puts("cntpct_el0:0x");
        put_long_hex(regvalue);
        unsigned long long cpu_counter=regvalue;
        puts("\r\n");
        __asm__ volatile(
            "mrs x0, cntp_cval_el0;"
            "mov %0, x0" : "=r"(regvalue)
        );
        puts("cntp_cval_el0:0x");
        put_long_hex(regvalue);
        puts("\r\n");
        __asm__ volatile(
            "mrs x0, cntp_tval_el0;"
            "mov %0, x0" : "=r"(regvalue)
        );
        puts("cntp_tval_el0:0x");
        put_long_hex(regvalue);
        puts("\r\n");
        __asm__ volatile(
            "mrs x0, cntfrq_el0;"
            "mov %0, x0" : "=r"(regvalue)
        );
        puts("cntfrq_el0:0x");
        put_long_hex(regvalue);
        puts("\r\n");

        puts("Seconds:");
        put_int((int)(cpu_counter/regvalue));
        puts("\r\n");

    }
    else{
        puts("Unsupported register\r\n");
        return 0;
    }

    return 0;
}

int cmd_async_IO_test(){
    char async_buf[MAX_BUF_LEN];
    int ticks = 150;

    uart_irq_on();

    uart_irq_puts("Async I/O test:");
    uart_irq_gets(async_buf);
    uart_irq_puts("You just typed:");
    uart_irq_puts(async_buf);
    // if this line is not added, the output will be mixed with the next command
    while(ticks--);

    uart_irq_off();
    return 0;
}

int cmd_init_buddy(){
    buddy_init();
    return 0;
}


int cmd_test(){
    int counter=0;
    int* arr[10];
    for(int i=0;i<10;i++){
        arr[i]=NULL;
    }
    for(int i=0;i<10;i++){
        int* doge=fr_malloc(0x1000);
        if(doge == NULL)break;
        counter++;
        arr[i]=doge;
    }

    for(int i=0;i<10;i++){
        if(arr[i] == NULL)break;
        fr_free(arr[i]);
    }
    puts("counter:0x");
    put_hex(counter);
    puts("\r\n");

    return 0;
}

int cmd_dy_malloc(){
    int* arr[100];
    for(int i=0;i<100;i++){
        arr[i]=dy_malloc(sizeof(int));
    }
    for(int i=0;i<100;i++){
        puts("address:");
        put_hex(arr[i]);
        puts("\r\n");
    }

    for(int i=0;i<100;i++){
        dy_free(arr[i]);
    }

    return 0;
}

int cmd_timer_output_switch(){
    if(timer_flag)timer_flag=0;
    else timer_flag=1;
    return 0;
}

int cmd_handler(char* cmd){
	if(!strcmp(cmd,"help"))cmd_help();
	else if(!strcmp(cmd,"hello"))cmd_hello();
	else if(!strcmp(cmd,"info"))cmd_info();
    else if(!strcmp(cmd,"reboot"))cmd_reboot();
    //else if(!strcmp(cmd,"getsp"))cmd_get_sp();
    else if(!strcmp(cmd,"getdtb"))cmd_get_dtb_addr();
    else if(!strcmp(cmd,"fdt_cpio"))cmd_fdt_traverse();
    else if(!strcmp(cmd,"ls"))cmd_ls();
    else if(!strcmp(cmd,"cat"))cmd_cat();
    else if(!strcmp(cmd,"malloc"))cmd_malloc();
    else if(!strcmp(cmd,"get_el"))cmd_get_el();
    else if(!strcmp(cmd,"getreg"))cmd_get_reg();
    else if(!strcmp(cmd,"loadimg"))cmd_loadimg();
    else if(!strcmp(cmd,"async"))cmd_async_IO_test();
    //else if(!strcmp(cmd,"reverse"))cmd_output_reverse();
    else if(!strcmp(cmd,"test"))cmd_test();
    else if(!strcmp(cmd,"switch"))cmd_timer_output_switch();
    else if(!strcmp(cmd,"buddy"))cmd_init_buddy();
    else if(!strcmp(cmd,"dy"))cmd_dy_malloc();
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


