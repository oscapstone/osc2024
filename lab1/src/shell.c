#include "uart.h"
#include "mbox.h"

#define MAX_COMMAND_LENGTH 50 // 定义最大命令长度

// reboot
#define PM_PASSWORD 0x5a000000
#define PM_RSTC         ((volatile unsigned int*)0x3F10001c)
#define PM_WDOG         ((volatile unsigned int*)0x3F100024)

int shellStart() {
    char command[MAX_COMMAND_LENGTH]; // 声明命令缓冲区
    uart_puts("Welcome to MyShell\n"); // 打印欢迎消息
    while(1) {
        uart_puts("> ");          // 显示命令提示符
        echo_input(command);      // 顯示keyin的值
        process_command(command); // 处理命令
        clear_commend_buffer(command, MAX_COMMAND_LENGTH);
    }

    return 0;
}

void process_command(char *command) {
    // 在这里你可以实现简单的命令解析和命令执行逻辑
    // 你可以根据命令的不同，实现不同的功能
    // 这里只是一个示例，仅处理"hello"命令
    if (my_strcmp(command, "hello") == 0) { // 如果输入的命令是"hello\n"
        uart_puts("Hello World!\n"); // 回应 "Hello World!"
    } else if (my_strcmp(command, "help") == 0) {
        uart_puts("Available commands:\n");
        uart_puts("hello\n");
        uart_puts("help\n");
        uart_puts("mailbox\n");
        uart_puts("reboot\n");
    } else if (my_strcmp(command, "mailbox") == 0) {
        get_mbox_info();
    } else if (my_strcmp(command, "reboot") == 0) {
        reboot();
    } else {
        uart_puts("unknow command\n"); // 未知命令
    }
}

int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

void clear_commend_buffer(char *command, int length) {
    for (int i = 0; i < length; i++) {
        command[i] = '\0';  // 将每个位置设置为 null 终止字符
    }
}

void echo_input(char *command) {
    char c;
    int i = 0;
    while (1) {
        c = uart_getc(); // 逐字符读取输入
        if (c == '\n') {
            uart_puts("\r\n"); // 添加回车换行
            command[i] = '\0'; // 在命令字符串末尾添加 NULL 字符
            break;
        }
        uart_send(c); // 将字符发送回终端
        command[i++] = c; // 将字符存入命令缓冲区
    }
}

void reboot() {
    uart_puts("Reboot in 3 seconds ...\r\n\r\n");
    *PM_RSTC = PM_PASSWORD | 0x20;  // full reset
    *PM_WDOG = PM_PASSWORD | 20000;  // number of watchdog tick
}