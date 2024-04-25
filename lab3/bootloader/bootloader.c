#include"header/bootloader.h"
#include"header/uart.h"
#include"header/utils.h"
// from linker script
extern char _start;
extern char _end;
void relocate(char *arg)
{
    unsigned long bootloader_size = (&_end - &_start);
    char *oldbootloader = (char *)&_start; //0x80000
    char *newbootloader = (char *)0x60000;

    unsigned long bl_ptr = 0;
    // copying
    while (bootloader_size--)
    {
        newbootloader[bl_ptr] = oldbootloader[bl_ptr];
        ++bl_ptr;
    }
    // run kernel in 0x60000
    void (*run)(char *) = (void (*)(char *))newbootloader;
    run(arg);
}
// can be verifying in gdb
void load_img(char *dtb_base) {
    // kernel start
    char *kernel = (char *)(0x80000);
    int kn_ptr = 0;

    // size 
    int idx = 0;
    char sz[50] = {};
    char c;
    
    // receiving str size
    while(1) {
        c = uart_get_char();
        // receive size end
        if(c == '\n') {
            sz[idx] = '\0';
            break;
        }    
        sz[idx++] = c;
    }
    // get kernel image size
    int size = atoi(sz);
    uart_send_str(sz);

    // receive kernel img
    while (size--)
        kernel[kn_ptr++] = uart_get_img_char();
    // test message
    uart_binary_to_hex((unsigned int) dtb_base);
    uart_send_str("\nKernel received\n");
    int r = 1000;
    while(r--){
        asm volatile("nop");
    }
    // run kernel and pass dtb base
    void (*run)(char *) = (void *)kernel;
    // get dtb loading address
    // run("0x60000");
    run(dtb_base);
}