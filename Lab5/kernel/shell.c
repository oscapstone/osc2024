#include "shell.h"

void print_menu(){
    async_uart_puts("\nhelp     : print this help menu");
    async_uart_puts("\nhello    : print Hello World!");
    async_uart_puts("\nmailbox  : print mailbox info");
    async_uart_puts("\nreboot   : reboot the device");  
    async_uart_puts("\ncat      : cat all files in file system");
    async_uart_puts("\nls       : list all files in file system");
    async_uart_puts("\nexec     : execute user program");
    async_uart_puts("\nasync    : async read/write");
    async_uart_puts("\ntimer    : test timer");
    async_uart_puts("\npreempt  : test preemption");
    async_uart_puts("\nalloc    : simple allocator");
    async_uart_puts("\nmalloc   : test malloc");
    async_uart_puts("\nfork     : test fork");
}

void print_rpi_info(){
    get_board_revision();
    get_memory_info();
}

void reboot(){
    reset(200);
}

void test_exec(){
    cpio_exec();
}

void test_timer(){

    set_timeout("\nthis will print after 6 second", 6);
    set_timeout("\nthis will print after 4 second", 4);
    
    void (*func)();
    func = print_current_time;
    sleep(func, 12);
}

void test_preempt(){
    
    add_task(print_current_time, 20);
    async_uart_puts("\nAfter Current Time");

    pop_task();
}

void test_simple_alloc(){
    async_uart_puts("\n----------");
    async_uart_puts("\nTesting allocating memory for \"short str\"");
    char* str1 = (char*)simple_alloc(10);
    strncpy("short str", str1, 10);

    async_uart_puts("\nstr1 = ");
    async_uart_puts(str1);

    async_uart_puts("\nAddress = 0x");
    async_uart_hex((uint32_t)str1);

    async_uart_puts("\n----------");
    async_uart_puts("\nTesting allocating memory for \"Long Long String in RPI\"");
    char* str2 = (char*)simple_alloc(30);
    strncpy("Long Long String in RPI", str2, 30);

    async_uart_puts("\nstr2 = ");
    async_uart_puts(str2);

    async_uart_puts("\nAddress = 0x");
    async_uart_hex((uint32_t)str2);
}

void test_malloc(){

    async_uart_puts("\n########## Test 1 ##########\n");

    void* ptr_1 = malloc((1 << 19));
    if (ptr_1 == 0)
        async_uart_puts("\nptr_1 Fail");

    async_uart_newline();

    void* ptr_2 = malloc((1 << 19));
    if (ptr_2 == 0)       
        async_uart_puts("\nptr_2 Fail");

    async_uart_newline();

    free(ptr_1);

    async_uart_puts("\n\n########## Test 2 ##########\n");

    void* ptr_3 = malloc(0x100607);
    if (ptr_3 == 0)
        async_uart_puts("\nptr_3 Fail");

    async_uart_newline();

    async_uart_puts("\n\n########## Test 3 ##########\n");

    void* ptr_4 = malloc(20);
    if (ptr_4 == 0)
        async_uart_puts("\nptr_4 Fail");

    async_uart_newline();

    void* ptr_5 = malloc(20);
    if (ptr_5 == 0)
        async_uart_puts("\nptr_5 Fail");

    async_uart_newline();

    free(ptr_4);

    async_uart_newline();

    free(ptr_5);

    async_uart_newline();

    free(ptr_3);

    async_uart_puts("\n\n########## Test 4 ##########\n");

    void* ptr_6 = malloc(2048);
    if (ptr_6 == 0)
        async_uart_puts("\nptr_6 Fail");

    async_uart_newline();

    void* ptr_7 = malloc(2048);
    if (ptr_7 == 0)
        async_uart_puts("\nptr_7 Fail");

    async_uart_newline();


    void* ptr_8 = malloc(2048);
    if (ptr_8 == 0)
        async_uart_puts("\nptr_8 Fail");

    async_uart_newline();

    free(ptr_7);

    async_uart_newline();

    async_uart_puts("\nFree ptr_7 one more time\n");
    free(ptr_7);

    async_uart_newline();

    free(ptr_8);

    async_uart_newline();

    free(ptr_6);

    async_uart_newline();
}

void shell(){

    async_uart_puts("\nAngus@Rpi3B+ > ");

    char cmd[256];

    read_input(cmd);

    if (strcmp(cmd, "help")){
        print_menu();
    }else if (strcmp(cmd, "hello")){
        async_uart_puts("\nHello World!");
    }else if (strcmp(cmd, "mailbox")){
        async_uart_puts("\nMailbox info :");
        print_rpi_info();
    }else if (strcmp(cmd, "reboot")){
        async_uart_puts("\nRebooting...");
        reboot();
    }else if (strcmp(cmd, "cat")){
        cpio_cat();   
    }else if (strcmp(cmd, "ls")){
        cpio_ls();
    }else if (strcmp(cmd, "alloc")){
        async_uart_puts("\nCannot use this allocator! (memory size decided)");
        // test_simple_alloc();
    }else if (strcmp(cmd, "exec")){
        test_exec();
    }else if (strcmp(cmd, "async")){
        async_uart_puts("\nAsync Enter > ");
        test_async_uart();
    }else if (strcmp(cmd, "timer")){
        test_timer();
    }else if (strcmp(cmd, "preempt")){
        test_preempt();
    }else if (strcmp(cmd, "malloc")){
        test_malloc();
    }else if (strcmp(cmd, "fork")){
        // new_user_thread(fork_test);
        thread_exec(fork_test, 0);
    }else{
        async_uart_puts("\nCommand Not Found");
    }


}

