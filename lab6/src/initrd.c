#include <stdint.h>

#include "initrd.h"
#include "alloc.h"
#include "mini_uart.h"
#include "string.h"
#include "c_utils.h"
#include "thread.h"
#include "exception.h"
#include "timer.h"
#include "utils.h"
#include "mmu.h"
#include "exec.h"

char *ramfs_base;
char *ramfs_end;


void enter_el0_run_user_prog(void *entry, char *user_sp);

static void user_prog_start(void)
{
    uart_send_string("User program start\n");
    enter_el0_run_user_prog((void *)0, (char *)0xffffffffeff0);

    // User program should call exit() to terminate
}

// Convert hexadecimal string to int
// @param s: hexadecimal string
// @param n: string length
static int hextoi(char *s, int n)
{
	int r = 0;
	while (n-- > 0) {
		r = r << 4;
		if (*s >= 'A')
			r += *s++ - 'A' + 10;
		else if (*s >= '0')
			r += *s++ - '0';
	}
	return r;
}

int cpio_newc_parse_header(cpio_t *this_header_pointer, char **pathname, unsigned int *filesize, char **data, cpio_t **next_header_pointer)
{
    // Ensure magic header 070701 
    // new ascii format
    if (strncmp(this_header_pointer->c_magic, CPIO_NEWC_HEADER_MAGIC, sizeof(this_header_pointer->c_magic)) != 0)
        return -1;

    // transfer big endian 8 byte hex string to unsinged int 
    // data size
    *filesize = hextoi(this_header_pointer->c_filesize, 8);

    // end of header is the pathname
    // header | pathname str | data
    *pathname = ((char *)this_header_pointer) + sizeof(cpio_t);

    // get file data, file data is just after pathname
    // header | pathname str | data
    // check picture on hackmd note
    unsigned int pathname_length = hextoi(this_header_pointer->c_namesize, 8);
    // get the offset to start of data
    // | offset | data
    unsigned int offset = pathname_length + sizeof(cpio_t);
    // pathname and data might be zero padding
    // section % 4 ==0
    offset = offset % 4 == 0 ? offset : (offset + 4 - offset % 4); // padding
    // header pointer + offset = start of data
    // h| offset | data
    *data = (char *)this_header_pointer + offset;

    // get next header pointer
    if (*filesize == 0)
        // hardlinked files handeld by setting filesize to zero
        *next_header_pointer = (cpio_t *)*data;
    else
    {
        // data size
        offset = *filesize;
        // move pointer to the end of data
        *next_header_pointer = (cpio_t *)(*data + (offset % 4 == 0 ? offset : (offset + 4 - offset % 4)));
    }

    // if filepath is TRAILER!!! means there is no more files.
    // end of archieve
    // empty filename : TRAILER!!!
    if (strncmp(*pathname, "TRAILER!!!", sizeof("TRAILER!!!")) == 0)
        *next_header_pointer = 0;

    return 0;
}

void initrd_list()
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    // current pointer
    cpio_t *header_pointer = (cpio_t *)(ramfs_base);

    // print every cpio pathname
    while (header_pointer)
    {
        // uart_send_string("header_pointer: ");
        // uart_hex((unsigned long)header_pointer);
        // uart_send_string("\n");
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        // if parse header error
        if (error)
        {
            uart_send_string("Error parsing cpio header\n");
            break;
        }
        // uart_send_string("header_pointer: ");
        // uart_hex((unsigned long)header_pointer);
        // uart_send_string("\n");
        // if this is not TRAILER!!! (last of file)
        if (header_pointer != 0){
            uart_send_string(filepath);
            uart_send_string("\n");
        }
    }
    return;
}


void initrd_cat(const char *target)
{
    char *filepath;
    char *filedata;
    unsigned int filesize;
    // current pointer
    cpio_t *header_pointer = (cpio_t *)(ramfs_base);

    // print every cpio pathname
    while (header_pointer)
    {
        // uart_send_string("header_pointer: ");
        // uart_hex((unsigned long)header_pointer);
        // uart_send_string("\n");
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        // if parse header error
        if (error)
        {
            // uart_printf("error\n");
            uart_send_string("Error parsing cpio header\n");
            break;
        }
        if (!strcmp(target, filepath))
        {
            for (unsigned int i = 0; i < filesize; i++)
                uart_send(filedata[i]);
            uart_send_string("\n");
            break;
        }
        // uart_send_string("header_pointer: ");
        // uart_hex((unsigned long)header_pointer);
        // uart_send_string("\n");
        // if this is not TRAILER!!! (last of file)
        if (header_pointer == 0){
            uart_send_string("File not found\n");
            break;
        }
    }
    return;
}

void initrd_callback(unsigned int node_type, char *name, void *value, unsigned int name_size){
    if(!strcmp(name, "linux,initrd-start")){
        ramfs_base = (char *)(unsigned long long)endian_big2little(*(unsigned int *)value);
        ramfs_base = (char *)PA2VA(ramfs_base);
        uart_send_string("ramfs_base: ");
        uart_hex((unsigned long)ramfs_base);
        uart_send_string("\n");
    }
    if(!strcmp(name, "linux,initrd-end")){
        ramfs_end = (char *)(unsigned long long)endian_big2little(*(unsigned int *)value);
        ramfs_end = (char *)PA2VA(ramfs_end);
        uart_send_string("ramfs_end: ");
        uart_hex((unsigned long)ramfs_end);
        uart_send_string("\n");
    }
}

void initrd_exec_prog(char* target) {
    el1_interrupt_disable();
    void* target_addr;
    char *filepath;
    char *filedata;
    unsigned int filesize;
    // current pointer
    cpio_t *header_pointer = (cpio_t *)(ramfs_base);

    // print every cpio pathname
    while (header_pointer)
    {
        // uart_send_string("header_pointer: ");
        // uart_hex((unsigned long)header_pointer);
        // uart_send_string("\n");
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        // if parse header error
        if (error)
        {
            // uart_printf("error\n");
            uart_send_string("Error parsing cpio header\n");
            break;
        }
        if (!strcmp(target, filepath))
        {
            uart_send_string("filesize: ");
            uart_hex(filesize);
            uart_send_string("\n");
            target_addr = kmalloc(filesize);
            memcpy(target_addr, filedata, filesize);
            break;
        }
        // uart_send_string("header_pointer: ");
        // uart_hex((unsigned long)header_pointer);
        // uart_send_string("\n");
        // if this is not TRAILER!!! (last of file)
        if (header_pointer == 0){
            uart_send_string("Program not found\n");
            return;
        }
    }
    uart_send_string("prog addr: ");
    uart_hex(target_addr);
    uart_send_string("\n");
    thread_t* t = create_thread(target_addr);
    el1_interrupt_enable();
    unsigned long spsr_el1 = 0x0; // run in el0 and enable all interrupt (DAIF)
    unsigned long elr_el1 = t -> callee_reg.lr;
    unsigned long user_sp = t -> callee_reg.sp;
    unsigned long kernel_sp = (unsigned long)t -> kernel_stack + T_STACK_SIZE;
    // "r": Any general-purpose register, except sp
    asm volatile("msr tpidr_el1, %0" : : "r" (t));
    asm volatile("msr spsr_el1, %0" : : "r" (spsr_el1));
    asm volatile("msr elr_el1, %0" : : "r" (elr_el1));
    asm volatile("msr sp_el0, %0" : : "r" (user_sp));
    asm volatile("mov sp, %0" :: "r" (kernel_sp));
    asm volatile("eret"); // jump to user program
    return;
}

void initrd_exec_syscall() {
    void* target_addr;
    char *filepath;
    char *filedata;
    unsigned int filesize;
    char* target = "vm.img";
    thread_t* t = get_current_thread();
    uart_send_string("current thread tid: ");
    uart_hex(t -> tid);
    uart_send_string("\n");
    // current pointer
    cpio_t *header_pointer = (cpio_t *)(ramfs_base);
    // print_running();
    // print every cpio pathname
    while (header_pointer)
    {
        uart_send_string("header_pointer: ");
        uart_hex((unsigned long)header_pointer);
        uart_send_string("\n");
        int error = cpio_newc_parse_header(header_pointer, &filepath, &filesize, &filedata, &header_pointer);
        // if parse header error
        if (error)
        {
            // uart_printf("error\n");
            uart_send_string("Error parsing cpio header\n");
            break;
        }
        if (!strcmp(target, filepath))
        {
            uart_send_string("filesize: ");
            uart_hex(filesize);
            uart_send_string("\n");
            t -> data = (void*)kmalloc(filesize);
            t -> data_size = filesize;
            uart_send_string("Copying user program\n");
            memcpy(t -> data, filedata, t -> data_size);
            uart_send_string("Finished\n");
            break;
        }
        // uart_send_string("header_pointer: ");
        // uart_hex((unsigned long)header_pointer);
        // uart_send_string("\n");
        // if this is not TRAILER!!! (last of file)
        if (header_pointer == 0){
            // uart_send_string("Program not found\n");
            return;
        }
    }
    uart_send_string("prog addr: ");
    uart_hex(t->data);
    uart_send_string("\n");
    uart_send_string("thread tid: ");
    uart_hex(t -> tid);
    uart_send_string("\n");

    for(int i=0;i<=SIGNAL_NUM;i++) {
        t -> signal_handler[i] = 0;
        t -> waiting_signal[i] = 0;
    }

    unsigned long spsr_el1 = 0x0; // run in el0 and enable all interrupt (DAIF)
    unsigned long elr_el1 = 0x0;
    unsigned long kernel_sp = (unsigned long)t -> kernel_stack + T_STACK_SIZE;
    // unsigned long kernel_sp = (unsigned long)kmalloc(T_STACK_SIZE) + T_STACK_SIZE;
    

    // t -> callee_reg.lr = user_prog_start;
    t -> page_table = pt_create();
    pt_map(t -> page_table, (void*)0, t -> data_size, 
           (void*)VA2PA(t -> data), PT_R | PT_W | PT_X); // map user program
    pt_map(t -> page_table, (void*)0xffffffffb000, T_STACK_SIZE,
        (void*)VA2PA(t -> user_stack), PT_R | PT_W); // map user stack

    pt_map(t->page_table, (void *)0x3c000000, 0x04000000,
            (void *)0x3c000000, PT_R | PT_W); // map mailbox
    
    uart_send_string("page table addr: ");
    uart_hex(t -> page_table);
    uart_send_string("\n");

    set_page_table(t);
    core_timer_enable();
    // el1_interrupt_enable();
    uart_send_string("exec user prog\n");
    exec_user_prog((void *)0, (char *)0xffffffffeff0, kernel_sp);
    // return;
    // // core_timer_enable();
    // // el1_interrupt_enable();
    
    // asm volatile("msr spsr_el1, %0" : : "r" (spsr_el1));
    // asm volatile("msr elr_el1, %0" : : "r" (elr_el1));
    // asm volatile("msr sp_el0, %0" : : "r" (user_sp));
    // asm volatile("mov sp, %0" :: "r" (kernel_sp));
    // // print_running();
    // // el1_interrupt_enable();
    // asm volatile("eret"); // jump to user program
    // // print_running();
}

void initrd_run_syscall() {
    // core_timer_disable();
    // el1_interrupt_disable();
    int tid = create_thread(initrd_exec_syscall) -> tid;
    thread_wait(tid);
}
