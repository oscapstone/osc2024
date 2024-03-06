#include "shell.h"
#include "mailbox.h"
#include "string.h"

typedef struct cmds_t {
	char* name;
    char* desc;
	int (*funct)(int, char **);
} cmds_t;


cmds_t cmds[] = {
	{"help",    "print help menu",      help},
    {"hello",   "print Hello World!",   hello},
    {"board",   "print board info",     board_info},
    {"reboot",  "reboot the device, usage: reboot <tick>", reboot},
};

int cmds_len = sizeof(cmds)/sizeof(cmds[0]);


int help(int argc, char *argv[]){
    for(cmds_t* cmd = cmds; cmd != cmds+cmds_len; cmd++)
        uart_printf("%s\t:\t%s\n", cmd->name, cmd->desc);
    return 0;
}

int hello(int argc, char *argv[]){
    uart_printf("Hello World!\n");
    return 0;
}

int board_info(int argc, char *argv[]){
    print_board_info();
    return 0;
}

#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

void reset(int tick) {                 // reboot after watchdog timer expire
    uart_printf("reboot after %d ticks\n", tick);
    set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    set(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset() {
    set(PM_RSTC, PM_PASSWORD | 0);  // full reset
    set(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}

int reboot(int argc, char *argv[]){
    unsigned int tick = 10;
    if(argc >= 2)
        tick = atoi(argv[1]);
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
        char *token = strtok(buf, ' ');
        while(token != 0){
            argv[argc++] = token;
            token = strtok(0, ' ');
        }
        
        if(argc){
            int flag = 1;
            for(cmds_t* cmd = cmds; cmd != cmds+cmds_len; cmd++){
                if(0 != strcmp(argv[0], cmd->name)) continue;
                (*cmd->funct)(argc, argv);
                flag = 0;
                break;
            }
            if(flag) uart_printf("Unknown command: %s\n", argv[0]);
        }   
	}
    return 0;
}