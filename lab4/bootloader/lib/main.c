


int relocated = 1;

char *dtb_base;

void main(char *arg)
{
    uart_init();
    // x0 
    dtb_base = arg;
    // relocate copies bootloader program from 0x80000 to 0x60000
    if (relocated) {
        relocated = 0;
        relocate(arg);
    }
    uart_puts("\x1b[2J\x1b[H");
    
    shell();
}