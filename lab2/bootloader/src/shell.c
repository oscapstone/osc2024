#include "mini_uart.h"

unsigned int is_visible(unsigned int c){
    if(c >= 32 && c <= 126){
        return 1;
    }
    return 0;
}

unsigned int strcmp(char* str1, char* str2){
    int i = 0;
    while(str1[i] != '\0' && str2[i] != '\0'){
        if(str1[i] != str2[i]){
            return 0;
        }
        i++;
    }
    if(str1[i] != '\0' || str2[i] != '\0'){
        return 0;
    }
    return 1;
}


void uart_recv_command(char *str){
    char c;
    int i = 0;
    while(1){
        c = uart_recv();
        if(c == '\n'){
            str[i] = '\0';
            break;
        } else if(c == 127 || c == 8){
            if(i > 0){
                i--;
                uart_send('\b');
                uart_send(' ');
                uart_send('\b');
            }
            continue;
        }
        if(is_visible(c)){
            str[i] = c;
            i++;
            uart_send(c);
        }
    }

}

int atoi(const char *s){
    int sign = 1;
    int i = 0;
    int result = 0;

    while(s[i] == ' ')
        i ++;
    
    if(s[i] == '-') {
        sign = -1;
        i++;
    }

    while(s[i] >= '0' && s[i] <= '9') {
        result = result * 10 + (s[i] - '0');
        i ++;
    }

    return sign * result;
}


void load_img(){
    char buf[16] = {0};
	for(int i = 0; i < 16; i++) {
		buf[i] = uart_recv();
		if (buf[i] == '\n') {
			buf[i] = '\0';
			break;
		}
	}

	uart_send_string("Kernel size: ");
	uart_send_string(buf);
	uart_send_string(" bytes\n");

    uart_send_string("Loading kernel...\n");
	unsigned int size = atoi(buf);
	char *kernel_addr = (char *)0x80000;
	while (size --) {
		*kernel_addr++ = uart_recv();
	}

	asm volatile(
        "mov x0, x10;"
        "mov x1, x11;"
        "mov x2, x12;"
        "mov x3, x13;"
        "mov x30, 0x80000;"
        "ret;"
    );

}


void shell(){
    uart_send_string("Welcome to Jayinnn's OSC bootloader!\n");
    while(1){
        uart_send_string("# ");
        char str[100];
        uart_recv_command(str);
        // uart_send_string(str);
        uart_send_string("\n");
        if(strcmp(str, "hello")){
            uart_send_string("Hello World!\n");
        } else if(strcmp(str, "load")){
            load_img();
            return;
        } else if(strcmp(str, "help")){
            uart_send_string("help\t: print this help menu\n");
            uart_send_string("hello\t: print Hello World!\n");
            uart_send_string("load\t: load kernel8.img\n");
        }
    }
}