#include "init_ramdisk.h"
#include "mini_uart.h"
#include "c_utils.h"
#include "alloc.h"

/*
The cpio archive format collects any number of files, directories, and
other file system objects (symbolic links, device nodes, etc.) into a
single stream of bytes.
Each file system in a cpio archive contains a header record.
The header is followed by the pathname of the entry 
(the length of the path_name is stored in the header)  
and any file data.  
The end of the archive is indicated	by a special record with the path_name "TRAILER!!!".
*/
void *initramdisk_addr;

cpio_newc_t *cpio_newc_header_parser(cpio_newc_t *cur_header, char **path_name, unsigned int *file_size, char **file_content)
{   
    cpio_newc_t *next_header;
    if (strncmp(cur_header->c_magic, CPIO_NEWC_HEADER_MAGIC, sizeof(cur_header->c_magic)) != 0) // check magic header 070701 
        return (cpio_newc_t *)-1;

    *file_size = hextoi(cur_header->c_filesize, 8);                                             // size of file -> convert big-e hex to uint
    *path_name = ((char *)cur_header) + sizeof(cpio_newc_t);                                    // header is followed by the path_name
    if (strncmp(*path_name, "TRAILER!!!", sizeof("TRAILER!!!")) == 0)                           // end of archieve -> filename = TRAILER!!!
        return 0;

    unsigned int path_name_len = hextoi(cur_header->c_namesize, 8); 
    unsigned int align_offset = sizeof(cpio_newc_t) + path_name_len;
    if (align_offset % 4 != 0)                                                                  // need to align 4 -> might have padding
        align_offset = (align_offset + 4 - align_offset % 4);
    *file_content = (char *)cur_header + align_offset;                                          // path_name is followed by the padding + the file_content
    
    if (*file_size == 0)                                                                        // hard-linked files
        next_header = (cpio_newc_t *)*file_content;
    else
    {
        align_offset = *file_size;
        if (align_offset % 4 != 0)                                                              // need to align 4 -> might have padding
            align_offset = (align_offset + 4 - align_offset % 4);
        next_header = (cpio_newc_t *)(*file_content + align_offset);
    }

    
    return next_header;
}

void init_ramdisk_ls()
{
    char *path_name;
    char *file_content;
    unsigned int file_size;
    cpio_newc_t *header_pointer = (cpio_newc_t *)(initramdisk_addr);
    // cpio_newc_t *header_pointer = (cpio_newc_t *)INITRAMFS_ADDR;

    while (header_pointer)                  // find all files in cpio archive
    {   
        header_pointer = cpio_newc_header_parser(header_pointer, &path_name, &file_size, &file_content);
        if (header_pointer == (cpio_newc_t *)-1)
        {
            uart_send_string("Error: init_ramdisk_ls: parsing cpio header...\r\n");
            break;
        }
        
        if (header_pointer != 0)            // if not end of archieve
        {          
            uart_send_string(path_name);
            uart_send_string("\r\n");
        }
    }
}


void init_ramdisk_cat(const char *file_name)
{
    char *path_name;
    char *file_content;
    unsigned int file_size;
    cpio_newc_t *header_pointer = (cpio_newc_t *)(initramdisk_addr);
    // cpio_newc_t *header_pointer = (cpio_newc_t *)INITRAMFS_ADDR;
   
    while (header_pointer)                  // find all files in cpio archive
    {
        header_pointer = cpio_newc_header_parser(header_pointer, &path_name, &file_size, &file_content);
        if (header_pointer == (cpio_newc_t *)-1)
        {
            uart_send_string("Error: init_ramdisk_cat: parsing cpio header...\r\n");
            break;
        }
        
        if (strcmp(file_name, path_name) == 0)
        {
            for (unsigned int i = 0; i < file_size; i++)
                uart_send(file_content[i]);
            uart_send_string("\r\n");
            break;
        }
        if (header_pointer == 0){                       // if end of archieve
            uart_send_string("Error: no such file...\r\n");
            break;
        }
    }
}

void init_ramdisk_callback(char *name, void *addr)
{
    if(!strcmp(name, "linux,initrd-start")){
        initramdisk_addr = (void *)(unsigned long long)endian_big2little(*(unsigned int *)addr);
        uart_send_string("\r\n\r\ninitramdisk_addr: ");
        uart_send_string_int2hex((unsigned long)initramdisk_addr);
        uart_send_string("\r\n");
    }
}

void init_ramdisk_load_user_prog(const char* file_name) 
{
    char* target_addr = (char*) 0x20000;

    char *path_name;
    char *file_content;
    unsigned int file_size;
    cpio_newc_t *header_pointer = (cpio_newc_t *)(initramdisk_addr);

    while (header_pointer)
    {
        header_pointer = cpio_newc_header_parser(header_pointer, &path_name, &file_size, &file_content);
        if (header_pointer == (cpio_newc_t *)-1)
        {
            uart_send_string("Error: init_ramdisk_cat: parsing cpio header...\r\n");
            return;
        }

        if (strcmp(file_name, path_name) == 0)
        {
            for (unsigned int i = 0; i < file_size; i++)
                *target_addr++ = file_content[i];
            break;
        }   

        if (header_pointer == 0){                       // if end of archieve
            uart_send_string("Error: no such file...\r\n");
            return;
        }
    }
    
    // set spsr_el1 to 0x3c0 and elr_el1 to the program’s start address.
    // 0b1111000000
    //   DAIFM
    // [3:0]: M = 0b0101 -> EL0t
    unsigned long spsr_el1 = 0x3c0;
    // set ELR_EL1(Exception Link Register) will hold the return address to which the processor should jump when the eret is executed.
    unsigned long elr_el1 = 0x20000;
    // set the user program’s stack pointer to a proper position by setting sp_el0
    unsigned long usr_sp = (unsigned long)simple_malloc(4096); // should plus 4096

    // first ':' -> no output operand
    // second ':' -> input -> "r" means any general-purpose register, except sp.
    // third ':' -> no modified register list
    asm volatile(
        "msr spsr_el1, %0;"
        "msr elr_el1, %1;"
        "msr sp_el0, %2;"
        "eret;"
        :
        : "r" (spsr_el1), "r" (elr_el1), "r" (usr_sp)
        :
    );
}