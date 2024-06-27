#include "../include/mini_uart.h"
#include "../include/shell.h"
#include "../include/string_utils.h"
#include "../include/mailbox.h"
#include "../include/reboot.h"
#include "../include/cpio.h"
#include "../include/mem_utils.h"
#include "../include/timer.h"
#include "../include/timer_utils.h"
#include "../include/exception.h"
#include "../include/sched.h"

#define BUFFER_SIZE 100

extern char _end;

void shell()
{
    while (1) {
        char buffer[BUFFER_SIZE];
        uart_send_string("> ");
        read_command(buffer);
        parse_command(buffer);
    }
}

void read_command(char *buffer)
{
    int index = 0;
    while (1)
    {
        buffer[index] = uart_recv();
        uart_send(buffer[index]);
        if (buffer[index] == '\r')
        {
            uart_send('\n');
            break;
        }
        index++;
    }
    buffer[index] = '\0';
}

void parse_command(char *buffer)
{
    // First, we need to deal with '\n' at the end before '\0' in buffer
    
    // Then, use my_strcmp to find which command it can be used
    if (my_strcmp(buffer, "help") == 0) {
        help();
    } else if (my_strcmp(buffer, "hello") == 0) {
        hello();
    } else if (my_strcmp(buffer, "info") == 0) {
        get_board_revision();
        get_base_address();
    } else if (my_strcmp(buffer, "reboot") == 0) {
        uart_send_string("rebooting...\r\n");
        reset(1000);
    } else if (my_strcmp(buffer, "ls") == 0) {
        cpio_ls();
    } else if (my_strcmp(buffer, "cat") == 0) {
        cpio_cat();
    } else if (my_strcmp(buffer, "load") == 0) {
        cpio_load_program();
    } else if (my_strcmp(buffer, "timer") == 0) {
        // // enable_timer_interrupt();
        // // TODO
        // char tmp_buffer[BUFFER_SIZE];
        // uart_send_string("Enter message: ");
        // read_command(tmp_buffer);
        // char msg[32]; 
        // my_strncpy(msg, tmp_buffer, 32);
        // uart_send_string("Enter duration: ");
        // read_command(tmp_buffer);
        // int duration = my_stoi(tmp_buffer);
        // add_timeout_event(msg, duration);
    } else if (my_strcmp(buffer, "async") == 0) {
        uart_async_demo();
    } else if (my_strcmp(buffer, "test") == 0){
        uart_hex(my_stoi("17"));
    } else if (my_strcmp(buffer, "malloc") == 0) {
        /* test malloc */
        // char *tmp = malloc(4);
        // if (!tmp) {
        // 	uart_send_string("HEAP overflow!!!\r\n");
        // 	return;
        // }
        // tmp[0] = '1';
        // tmp[1] = '2';
        // tmp[2] = '3';
        // tmp[3] = '\0';
        // uart_send_string(tmp);
        // uart_send_string("\r\n");
        
        // /* test strlen */
        // uart_send_string("The length of tmp is: ");
        // uart_hex(my_strlen(tmp));
        // uart_send_string("\r\n");
    } else if (my_strcmp(buffer, "heap_limit") == 0){ 
        printf("heap_start: %8x\n", &_end);
        printf("heap_end:   %8x\n", show_heap_end());
    } else if (my_strcmp(buffer, "buddy_init") == 0) {
        buddy_system_init();
    } else if (my_strcmp(buffer, "page_alloc") == 0){
        char tmp_buffer[BUFFER_SIZE];
        uart_send_string("Enter number: ");
        read_command(tmp_buffer);
        char *page_addr = (char *)page_frame_allocate(my_stoi(tmp_buffer));
        printf("The start address of page: %8x\n", page_addr);
    } else if (my_strcmp(buffer, "page_free") == 0) {
        char tmp_buffer[BUFFER_SIZE];
        uart_send_string("Enter addr: ");
        read_command(tmp_buffer);
        page_frame_free((char *)hexstr2val(tmp_buffer, 8));
    } else if (my_strcmp(buffer, "layout") == 0) {
        show_memory_layout();
    } else if (my_strcmp(buffer, "d_alloc_init") == 0) {
        dynamic_allocator_init();
    } else if (my_strcmp(buffer, "chunk_alloc") == 0) {
        char tmp_buffer[BUFFER_SIZE];
        uart_send_string("Enter number: (bytes)");
        read_command(tmp_buffer);
        char *addr = (char *)chunk_alloc(my_stoi(tmp_buffer));
        printf("The start address of chunk: %8x\n", addr);
    } else if (my_strcmp(buffer, "chunk_free") == 0) {
        char tmp_buffer[BUFFER_SIZE];
        uart_send_string("Enter addr: ");
        read_command(tmp_buffer);
        chunk_free((char *)hexstr2val(tmp_buffer, 8));
    } else if (my_strcmp(buffer, "task_head") == 0) {
        show_task_head();
    } else{
        uart_send_string("command ");
        uart_send_string(buffer);
        uart_send_string(" not found\r\n");
    }
}

void help()
{
    uart_send_string("help          print all available commands\r\n");
    uart_send_string("hello         print Hello World!\r\n");
    uart_send_string("info          print board revision and memory base address and size\r\n");
    uart_send_string("reboot        reboot the rpi3b+\r\n");
    uart_send_string("ls            show all files in rootfs\r\n");
    uart_send_string("cat           print out the content of specific file\r\n");
    uart_send_string("load          load user program and execute\r\n");
    uart_send_string("timer         add timer event\r\n");
    uart_send_string("async         async uart demo\r\n");
    uart_send_string("test          test the function my_atoi\r\n");
    uart_send_string("malloc        try to print the content of malloc\r\n");
    uart_send_string("heap_limit    try to print out the limit of heap\r\n");
    uart_send_string("buddy_init    try to initialize the buddy system\r\n");
    uart_send_string("page_alloc    allocate the page by the number (KB)\r\n");
    uart_send_string("page_free     free the page by its address\r\n");
    uart_send_string("layout        show memory layout of buddy system\r\n");
    uart_send_string("d_alloc_init  dynamic allocator init\r\n");
    uart_send_string("chunk_alloc   allocate chunk to user\r\n");
    uart_send_string("chunk_free    free the chunk\r\n");
    uart_send_string("task_head     show address of init_task\r\n");
}

void hello()
{
    uart_send_string("Hello World!\r\n");
}

