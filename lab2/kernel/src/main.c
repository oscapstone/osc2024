#include "uart1.h"
#include "shell.h"
#include "heap.h"
#include "u_string.h"
#include "dtb.h"

extern char* dtb_ptr; // it's the address of dtb and it declared in main.c



/* x0-x7 are argument registers.
   x0 is now used for dtb */
void main(char* arg){
    char input_buffer[CMD_MAX_LEN];

    dtb_ptr = arg;
    /*
        traverse_device_tree() is on osdi/osc2023/lab2/kernel/src/dtb.c
        dth_callback_initramfs() is on osdi/osc2023/lab2/kernel/src/dth.c
    */
    traverse_device_tree(dtb_ptr, dtb_callback_initramfs);

    uart_init();
    uart_puts("loading dtb from: 0x%x\n", dtb_ptr);
    cli_print_banner();

    while(1){
        cli_cmd_clear(input_buffer, CMD_MAX_LEN);
        uart_puts("# ");
        cli_cmd_read(input_buffer);
        cli_cmd_exec(input_buffer);
    }
}
