#include "../include/command.h"



extern char *cpio_addr;
extern int core_timer_flag;

void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}


void cmd_help(int argc, char **argv) {
    uart_puts("hello       : print Hello World!\n");
    uart_puts("reboot      : reboot the device\n");
    uart_puts("ls          : show file in rootfs\n");
    uart_puts("cat         : show file content\n");
    uart_puts("malloc      : malloc demp\n");
    uart_puts("rup         : run user program\n");
    uart_puts("async       : async UART read/write\n");
    uart_puts("sto         : set time out\n");
    uart_puts("cctf        : change core timer flag\n");
    uart_puts("ttm         : test timer multiplexing\n");
    uart_puts("tci         : test concurrent I/O devices handling\n");
}

command_t commands[] = {
    {"help", "print this help menu", cmd_help},
    {"hello", "print Hello World!", cmd_hello},
    {"reboot", "reboot the device", cmd_reboot},
    {"ls", "show file in rootfs", cmd_ls},
    {"cat", "show file content", cmd_cat},
    {"malloc", "malloc demo", cmd_malloc},
    {"rup", "run user program", cmd_rup},
    {"async", "async UART read/write", cmd_async},
    {"sto", "set time out", cmd_sto},
    {"cctf", "change timer type", cmd_cctf},
    {"ttm", "test timer multiplexing", cmd_ttm},
    {"tci", "test concurrent I/O devices handling", cmd_tci},
    {"atest", "allocator test", cmd_atest},
    {NULL, NULL, NULL} // Sentinel value to mark the end of the array
};

void cmd_hello(int argc, char **argv) {
    uart_puts("Hello World!\n");
}

void cmd_reboot(int argc, char **argv) {
    uart_puts("Start rebooting...\n");
    set(PM_RSTC, PM_PASSWORD | 0x20);
}

void cmd_ls(int argc, char **argv) {
    cpio_ls(cpio_addr);
}

void cmd_cat(int argc, char **argv){
    uart_puts("Filename: ");
    char *filename = get_string();
    if (filename == NULL) {
        uart_puts("Error: Invalid filename\n");
        return;
    }
    cpio_cat(cpio_addr,filename);
}

void cmd_malloc(int argc, char **argv) {
    char *ptr1 = simple_malloc(7);
    ptr1[0]='M';
    ptr1[1]='A';
    ptr1[2]='L';
    ptr1[3]='L';
    ptr1[4]='O';
    ptr1[5]='C';
    ptr1[6]='\0';
    uart_puts(ptr1);
    uart_puts("\n");
}

void cmd_rup(int argc, char **argv) {
    uart_puts("Filename: ");
    char *filename = get_string();
    if (filename == NULL) {
        uart_puts("Error: Invalid filename\n");
        return;
    }
    char *file_addr = cpio_find(cpio_addr,filename);
    //uart_hex(file_addr);
    //uart_puts("\n");
    void *stack_addr = simple_malloc(2048);  
    //make sure stack_addr and file_addr not NULL
    if(stack_addr && file_addr){
        write_register(spsr_el1,0x3c0);
        write_register(elr_el1,file_addr);
        write_register(sp_el0,stack_addr);
        asm volatile("eret");    
    }
}

void cmd_async(int argc, char **argv){

    /*enable uart interrupts*/

    // Enable mini second level interrupt controller’s 
    *((volatile unsigned int*)ENABLE_IRQS_1) |= (1 << AUX_IRQ);

    // Enable mini UART receive interrupt to start reading the data
    *((volatile unsigned int*)AUX_MU_IER) |= 0x01;


    //uart_puts_async("Async UART testing\n");
    uart_puts_async("Type in something\n");

    char buffer_async[TMP_BUFFER_LEN];
    strset(buffer_async, 0, TMP_BUFFER_LEN);

    get_string_async(buffer_async);
    uart_puts_async("\nWhat you type in is :\n");
    uart_puts_async(buffer_async);
    uart_puts_async("\n");
 
    


    /*disable uart interrupts*/
    
    // Disable mini second level interrupt controller’s 
    *((volatile unsigned int*)ENABLE_IRQS_1) &= ~(1 << AUX_IRQ);

    // Disable mini UART receive interrupts to stop reading the data
    *((volatile unsigned int*)AUX_MU_IER) &= ~(0x1);
    
}

void cmd_cctf(int argc, char **argv){
    if(core_timer_flag==0){
        uart_puts("enable core0 Timer\n");
        // enable core0 timer interrupt
        *((volatile uint32_t*)CORE0_TIMER_IRQ_CTRL) = (0x2);
        core_timer_flag=1;
    } 
    else{
        uart_puts("disable core0 Timer\n");
        // disnable core0 timer interrupt
        *((volatile unsigned int*)CORE0_TIMER_IRQ_CTRL) = (0x0);
        core_timer_flag=0;
    } 
}

void cmd_sto(int argc, char **argv){

    if (argc < 3) { // Ensure at least two arguments are present
        uart_puts("need to send massage and second\n");
        return;
    }
    unsigned long long cur_cnt, cnt_freq;
    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
    );
    uart_puts("current time:\n");
    uart_hex(cur_cnt / cnt_freq);
    uart_send('\n');

    setTimeout(argv[1], atoi(argv[2]));

}

void cmd_ttm(int argc, char **argv){

    unsigned long long cur_cnt, cnt_freq;
    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
    );
    uart_puts("current time:\n");
    uart_hex(cur_cnt / cnt_freq);
    uart_send('\n');

    char test_tesk[6];
    test_tesk[0]='t';
    test_tesk[1]='a';
    test_tesk[2]='s';
    test_tesk[3]='k';
    test_tesk[4]='1';
    test_tesk[5]='\0';
    
    for(int i=1; i<argc; i++){
        test_tesk[4] = i + '0';
        setTimeout(test_tesk, atoi(argv[i]));
    }
}


void cmd_tci(int argc, char **argv){
    
    task_t* newtask1 = create_task(test_tesk1, 0);
    task_t* newtask2 = create_task(test_tesk2, 2);
    task_t* newtask3 = create_task(test_tesk3, 1);

    add_task_to_queue(newtask1);
    add_task_to_queue(newtask2);
    add_task_to_queue(newtask3);

    ExecTasks();
}

void cmd_atest(void){
    uart_send('\n');

    show_available_page();
    show_first_available_block_idx();
    uart_send('\n');
    uart_getc();
    
    uart_puts("malloc 2*4096\n");
    void* x = dma_malloc(2*4096);
    show_available_page();
    show_first_available_block_idx();
    uart_send('\n');
    uart_getc();

    uart_puts("malloc 2*4096\n");
    void* y = dma_malloc(2*4096);
    show_available_page();
    show_first_available_block_idx();
    uart_send('\n');
    uart_getc();

    uart_puts("free 2*4096\n");
    dma_free(x,2*4096);
    show_available_page();
    show_first_available_block_idx();
    uart_send('\n');
    uart_getc();

    uart_puts("free 2*4096\n");
    dma_free(y,2*4096);
    show_available_page();
    show_first_available_block_idx();
    uart_send('\n');
    uart_getc();

    uart_puts("malloc 64\n");
    void* a = dma_malloc(64);
    show_available_page();
    uart_getc();
    uart_puts("free 64\n");
    dma_free(a,64);
    uart_puts("malloc 64\n");
    void* b = dma_malloc(64);
    uart_send('\n');
}