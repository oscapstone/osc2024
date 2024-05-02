
extern char *dtb_base;
void load() {
    // kernel start
    char *kernel = (char *)(0x80000);
    int kn_ptr = 0;

    // size 
    int idx = 0;
    char sz[50] = {};
    char c;
    // receiving str size
    while(1) {
        c = uart_getc();
        // receive size end
        if(c == '\n') {
            sz[idx] = '\0';
            break;
        }    
        sz[idx++] = c;
    }
    // get kernel image size
    int size = atoi(sz);
    uart_puts(sz);

    // receive kernel img
    while (size--)
        kernel[kn_ptr++] = uart_getc();
    // test message
    uart_puts("Kernel received \n");
    // run kernel and pass dtb base
    void (*run)(char *) = (void *)kernel;
    // get dtb loading address
    run(dtb_base);

}