#include "uart.h"
#include "io.h"
#include "shell.h"
#include "devtree.h"

int main(){

    uart_init();
    fdt_traverse(initramfs_callback);
    print_str("\nLogin Shell");

    while(1){
        shell();
    }

    return 0;
}