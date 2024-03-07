#include "kernel/shell.h"

void my_shell(){
    char buf[MAX_BUF_LEN];
    int buf_index;
    char input_char;

    while(1){
        buf_index = 0;
        string_set(buf, 0, MAX_BUF_LEN);
        uart_puts("# ");

        while(1){
            input_char = uart_getc();
            // Get non ASCII code
            if(input_char > 127 || input_char < 0){
                //uart_puts("\nwarning: Get non ASCII code\n");
                continue;
            }
            if(buf_index < MAX_BUF_LEN)
                buf[buf_index++] = parse(input_char);
            // should replace with parsed char
            uart_putc(input_char);
            // when receving ENTER
            if(input_char == '\n'){
                // add EOF after '\n'
                buf[buf_index] = '\0';
                break;
            }
        }

        if(buf_index >= MAX_BUF_LEN)
            uart_puts("Warning: buffer is full, command may not correct\n");

        if(!string_comp(buf, "help")){
            uart_puts("help     :Print this help menu\n");
            uart_puts("hello    :Print Hello World!\n");
            uart_puts("info     :Get revision and memory\n");
            uart_puts("reboot   :Reboot the device\n");
        }
        else if(!string_comp(buf, "hello")){
            uart_puts("Hello World!\n");
        }
        else if(!string_comp(buf, "info")){
            //uart_puts("Mailbox function is still woring on\n");
            /*if(mailbox_call()){
                get_board_revision();
                uart_puts("My board revision is: ");
                uart_b2x(mailbox[5]);
                uart_puts("\r\n");
            }*/
            get_board_revision();
            get_arm_mem();
        }
        else if(!string_comp(buf, "reboot")){
            //uart_puts("Reboot function is still woring on\n");
            uart_puts("Rebooting...\n");
            // after 1000 ticks, start resetting
            reset(1000);
        }
        else if(!string_comp(buf, "ls")){
            cpio_ls();
        }
        else if(!string_comp(buf, "cat")){
            //uart_puts("cat is still working on\n");
            buf_index = 0;
            string_set(buf, 0, MAX_BUF_LEN);
            
            uart_puts("Filename: ");

            while(1){
                input_char = uart_getc();
                // Get non ASCII code
                if(input_char > 127 || input_char < 0){
                    //uart_puts("\nwarning: Get non ASCII code\n");
                    continue;
                }
                if(buf_index < MAX_BUF_LEN)
                    buf[buf_index++] = parse(input_char);
                // should replace with parsed char
                uart_putc(input_char);
                // when receving ENTER
                if(input_char == '\n'){
                    // add EOF after '\n'
                    buf[buf_index] = '\0';
                    break;
                }
            }

            cpio_cat(buf);

            continue;
        }
        else{
            uart_puts("Unknown Command: ");
            uart_puts(buf);
        }
    }
}