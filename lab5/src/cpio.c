#include "../include/cpio.h"
#include "../include/mini_uart.h"
#include "../include/string_utils.h"
#include "../include/shell.h"
#include "../include/mem_utils.h"
#include "../include/mm.h"
#include "../include/fork.h"

extern char *cpio_addr;
#define KSTACK_SIZE 0x2000
#define USTACK_SIZE 0x2000

static void cpio_load(char *file_addr, unsigned int file_size)
{
    /* first, compute the size */
    printf("File size = %d byte\n", file_size);

    /* second, how many KB it has */
    uint32_t remainder = file_size % 4096;
    if (remainder)
        remainder = 1;
    uint32_t kilo_byte = (file_size >> PAGE_SHIFT) + remainder;
    printf("File size = %d KB\n", kilo_byte);

    /* third, allocate spaces for file */
    void *target = page_frame_allocate(kilo_byte);
    memcpy(target, file_addr, file_size);
    move_to_user_mode((unsigned long)target);
}

void cpio_ls()
{
    /* 1. Get the address of cpio_address. */
    char *addr = (char *)cpio_addr;

    /* 2. Use while loop to check the pathname of the file is "TRAILER!!!" or not. */
    while (my_strcmp((addr + sizeof(struct cpio_header)), "TRAILER!!!") != 0) {

        /* 3. calculate the namesize. */
        struct cpio_header *header = (struct cpio_header *)addr;
        unsigned int namesize = hexstr2val((char *)header->c_namesize, 8);
        // uart_hex(namesize);

        /* 4. let pointer point to the start of filename. */
        addr += sizeof(struct cpio_header);
        uart_send_string(addr);
        uart_send_string("\r\n");

        /* 5. align the address to the start of the file. */
        addr += namesize;
        addr = mem_align(addr, 4);

        /* 6. calculate the filesize. */
        unsigned int filesize = hexstr2val((char *)header->c_filesize, 8);
        //uart_hex(filesize);
        
        // try to print out the file.
        // uart_send_string(addr);
        // break;

        /* 7. align the address to the start of another file header. */
        addr += filesize;
        addr = mem_align(addr, 4);
    }

}

void cpio_cat()
{
    /* 1. Get the address of cpio_address and set up the flag. */
    char *addr = (char *)cpio_addr;
    int flag = 0;

    /* 2. Wait user enter the file name. */
    char buffer[100];
    uart_send_string("Enter file name: ");
    read_command(buffer);
    // uart_send_string(buffer);

    /* 3. Use while loop to check the pathname of the file is "TRAILER!!!" or not. */
    while (my_strcmp((addr + sizeof(struct cpio_header)), "TRAILER!!!") != 0) {

        /* 4. Calculate the namesize. */
        struct cpio_header *header = (struct cpio_header *)addr;
        unsigned int namesize = hexstr2val((char *)header->c_namesize, 8);
        // uart_hex(namesize);

        /* 5. Let pointer point to the start of filename. */
        addr += sizeof(struct cpio_header);
        // uart_send_string(addr);
        // uart_send_string("\r\n");

        /* 6. If the file name is the same as user input, flag -> 1. */
        if (my_strcmp(addr, buffer) == 0) 
            flag = 1;

        /* 7. Align the address to the start of the file. */
        addr += namesize;
        addr = mem_align(addr, 4);

        /* 8. Calculate the filesize. */
        unsigned int filesize = hexstr2val((char *)header->c_filesize, 8);
        //uart_hex(filesize);
        
        /* 9. Print out the file content. */
        if (flag) {
            uart_send_string(addr);
            uart_send_string("\r\n");
            break;
        }

        /* 10. align the address to the start of another file header. */
        addr += filesize;
        addr = mem_align(addr, 4);
    }

    /* If flag == 0, print out message. */
    if (!flag)
        uart_send_string("Please enter the correct filename. \r\n");

}

void cpio_load_program()
{
    /* 1. Get the address of cpio_address, and create a variable to get filesize of target file. */
    char *addr = (char *)cpio_addr;
    unsigned int size = 0;

    /* 2. Wait user enter the file name. */
    char buffer[100];
    uart_send_string("Enter file name: ");
    read_command(buffer);
    // uart_send_string(buffer);

    /* 3. Use while loop to check the pathname of the file is "TRAILER!!!" or not. */
    while (my_strcmp((addr + sizeof(struct cpio_header)), "TRAILER!!!") != 0) {

        /* 4. Calculate the namesize. */
        struct cpio_header *header = (struct cpio_header *)addr;
        unsigned int namesize = hexstr2val((char *)header->c_namesize, 8);
        // uart_hex(namesize);

        /* 5. Let pointer point to the start of filename. */
        addr += sizeof(struct cpio_header);
        // uart_send_string(addr);
        // uart_send_string("\r\n");

        /* 6. If the file name is the same as user input, flag -> 1. */
        if (my_strcmp(addr, buffer) == 0) {
            // uart_send_string(addr);
            // uart_send_string("\r\n");
            addr += namesize;
            addr = mem_align(addr, 4);
            size = hexstr2val((char *)header->c_filesize, 8);
            break;
        }

        /* 7. Align the address to the start of the file. */
        addr += namesize;
        addr = mem_align(addr, 4);

        /* 8. Calculate the filesize. */
        unsigned int filesize = hexstr2val((char *)header->c_filesize, 8);
        //uart_hex(filesize);

        /* 9. align the address to the start of another file header. */
        addr += filesize;
        addr = mem_align(addr, 4);
    }

    /* 10. Now we have start address of the content of img file, load the content to specific address. */
    // uart_hex(size);
    // uart_send_string("\r\n");

    // asm volatile("mov x0, 0x3c0  \n");
    // asm volatile("msr spsr_el1, x0   \n");
    // asm volatile("msr elr_el1, %0    \n" ::"r"(addr));
    // asm volatile("msr sp_el0, %0    \n" ::"r"(addr + USTACK_SIZE));
    // asm volatile("eret   \n");

    /* 11. use static function to load program and execute */
    cpio_load(addr, size);
}