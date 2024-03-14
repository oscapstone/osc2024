#include "shell.h"
#include "uart.h"
#include "mailbox.h"
#include <stddef.h>

// extern volatile unsigned int mailbox[8];

/*
    shell
*/
void shell() {
    while(1) {
        char buffer[100];
        uart_puts("# ");
        read_inst(buffer);
        compare_inst(buffer);
    }
}

void read_inst(char* buffer){
    int i = 0;
    while(1){
        buffer[i] = uart_getc();
        uart_send(buffer[i]);
        if(buffer[i] == '\n'){
            buffer[i] = '\0';
            buffer[i+1] = '\n';
            break;
        }
        i++;
    }
}

void compare_inst(char* buffer){
    char* input = buffer;
    newline2end(input);
    if(str_cmp(input, "help") == 0){
        uart_puts("help     :");
        uart_puts("print this help menu\r\n");
        uart_puts("hello    :");
        uart_puts("print Hello World!\r\n");
        uart_puts("info     :");
        uart_puts("show board revision and ARM memory base and size\r\n");
        uart_puts("reboot   :");
        uart_puts("reboot the device\r\n");
    }
    else if(str_cmp(input, "hello") == 0){
        uart_puts("Hello World!\r\n");
    }
    else if(str_cmp(input, "info") == 0){
        // if(mailbox_call()){
            get_board_revision();
            uart_puts("Board revision: ");
            uint_to_hex(mailbox[5]);
            uart_puts("\r\n");
            get_arm_memory();
            uart_puts("Arm base address: ");
            uint_to_hex(mailbox[5]);
            uart_puts("\r\n");
            uart_puts("Arm memory size: ");
            uint_to_hex(mailbox[6]);
            uart_puts("\r\n");
        // }
    }
    else if(str_cmp(input, "reboot") == 0){
        uart_puts("reboot test\r\n");
        reset(100);
    }
    else{
        uart_puts("error\r\n");
    }
}

/* 
    string part
*/
int str_cmp(char *str1, char *str2)
{
    char ch1, ch2;
    do
    {
        ch1 = (char)*str1++;
        ch2 = (char)*str2++;
        if (ch1 == '\0' || ch2 == '\0')
        {
            return ch1 - ch2;
        }
    } while (ch1 == ch2);
    return ch1 - ch2;
}

void newline2end(char *str)
{
    while (*str != '\0')
    {
        if (*str == '\n')
        {
            *str = '\0';
            return;
        }
        str++;
    }
}
void uint_to_hex(unsigned int num) {
    unsigned int n;
    int c;
    uart_puts("0x");
    for(int i = 28;i >= 0;i-=4){
        n = (num >> i) & 0xF;
        n+= (n > 9? 0x57:0x30);
        uart_send(n);
    }
}

// void uint_to_hex(unsigned int num) {
//     unsigned int tmp = num;
//     int bit = 0;
//     int divider = 1;

//     char* str = ;
//     char* str_start = str;
    
//     *str = '0';
//     *str++;
//     *str = 'x';
//     *str++;
//     if (num == 0){
//         for (int i = 0;i < 8;i++){
//             *str = '0';
//             str++;
//         }
//     }
//     else {
//         while (tmp > 0){
//             tmp /= 16;
//             bit++;
//             divider*=16;
//         }
//         tmp = num;
//         divider /= 16;
//         for (int i = 8;i > bit;i--){
//             *str = '0';
//             str++;
//         }
//         for (int i = bit; i > 0;i--){
//             if(tmp / divider >= 10){
//                 *str = 'A' + tmp/divider - 10;
//             }
//             else{
//                 *str = '0' + tmp/divider;
//             }
//             tmp %= divider;
//             divider /= 16;
//             str++;
//         }
//     }
//     *str = '\0';
//     uart_puts(str_start);
// }

/* 
    reboot
*/
void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

void reset(int tick) {                 // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    set(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset() {
    set(PM_RSTC, PM_PASSWORD | 0);  // full reset
    set(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}