#include "shell.h"
#include "mailbox.h"
#include "m_string.h"
#include "initramfs.h"
#include "mm.h"

int help(int argc, char *argv[]){
    uart_printf(
        "help:    print help menu\n"
        "hello:   print Hello World!\n"
        "board:   print board info\n"
        "reboot:  reboot the device, usage: reboot <tick>\n"
        "ls:      list directory\n"
        "cat:     dump text in <filepath>\n"
        );
    return 0;
}

int hello(int argc, char *argv[]){
    uart_printf("Hello World! I'm main kernel!\n");
    return 0;
}

int board_info(int argc, char *argv[]){
    print_board_info();
    return 0;
}

int ls(int argc, char *argv[]){
    cpio_file file;
    if(cpio_get_start_addr(&file.nextfile))
        return -1;
    while(0 == cpio_parse(&file)){
        uart_printf("%s\n", file.filename);
    }
    return 0;
}

int cat(int argc, char *argv[]){
    if(argc < 2){
        uart_printf("Usage: cat <filepath>\n");
        return -1;
    }
    cpio_file file;
    if(cpio_get_start_addr(&file.nextfile))
        return -1;
    while(0 == cpio_parse(&file)){
        if(0 == m_strcmp(file.filename, argv[1])){
            for(int i=0; i<file.filesize; ++i)
                uart_printf("%c", file.filedata[i]);
            uart_printf("\n");
            return 0;
        }
    }
    uart_printf("Not found!: %s\n", argv[1]);
    return -1;
}

void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

void reset(int tick) {
    uart_printf("reboot after %d ticks\n", tick);
    set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    set(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset() {
    uart_printf("cancel reboot \n");
    set(PM_RSTC, PM_PASSWORD | 0);  // full reset
    set(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}


int reboot(int argc, char *argv[]){
    unsigned int tick = 10;
    if(argc >= 2)
        tick = m_atoi(argv[1]);
    if(tick < 0)
        cancel_reset();
    else
        reset(tick);
    return 0;
}

int shell_loop(){
    while (1) {	
		char buf[256];
        unsigned int len = 0;
		uart_printf("$ ");
		while(len < 256){
			buf[len] = uart_recv();
            if(buf[len] == 127){
                if(len == 0) continue;
                uart_printf("\b \b");
                len--;
            }
            else{
			    uart_printf("%c", buf[len]);
                len++;
                if(buf[len-1] == '\n'){
                    buf[len-1] = '\0';
				    break;
			    }
            }
		}
        
        unsigned int argc = 0;
        char *argv[32];
        char *next = buf;
        char *token = m_strtok(buf, ' ', &next);
        while(*token != 0){
            argv[argc++] = token;
            token = m_strtok(next, ' ', &next);
        }

        if(argc == 0)
            continue;
        else if(0 == m_strcmp(argv[0], "help"))
            help(argc, argv);
        else if(0 == m_strcmp(argv[0], "hello"))
            hello(argc,argv);
        else if(0 == m_strcmp(argv[0], "board"))
            board_info(argc, argv);
        else if(0 == m_strcmp(argv[0], "reboot"))
            reboot(argc, argv);
        else if(0 == m_strcmp(argv[0], "ls"))
            ls(argc, argv);
        else if(0 == m_strcmp(argv[0], "cat"))
            cat(argc, argv);
        else
            uart_printf("Unknown command: %s\n", argv[0]);
	}
    return 0;
}