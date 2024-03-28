#include "uart1.h"
#include "shell.h"
#include "heap.h"
#include "utils.h"
#include "dtb.h"

extern char* dtb_ptr;//指向DT的地址

void main(char* arg){//arg:指向bootloader傳入的參數
    char input_buffer[CMD_MAX_LEN];

    dtb_ptr = arg;
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs);//初始化RAM系統

    uart_init();//初始化uart，通信
    uart_puts("loading dtb from: 0x%x\n", arg);
    cli_print_banner();

    while(1){
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_puts("# ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
}


/* Initial Ramdisk 與 Device Tree 之間的關係
bootloader首先加載DTB，這是DT的二進至形式

隨後bootloader加載Initial Ramdisk與kernel到memory

bootloader將DTB與控制權交給kernel

kernel讀取DTB了解硬體配置，還有利用Initial Ramdisk來初始化硬體

並且製作簡單的文件系統
*/

/* 簡易文件系統
利用CPIO與跟目錄rootfs

先創目錄rootfs
創建文件file0.txt file1.txt
歸檔到CPIO

傳遞這個CPIO文件給kernel
*/