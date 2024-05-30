#include <stdint.h>

#include "initrd.h"
#include "alloc.h"
#include "mini_uart.h"
#include "string.h"
#include "c_utils.h"

char *ramfs_base;
char *ramfs_end;


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
        uart_send_string("ramfs_base: ");
        uart_hex((unsigned long)ramfs_base);
        uart_send_string("\n");
    }
    if(!strcmp(name, "linux,initrd-end")){
        ramfs_end = (char *)(unsigned long long)endian_big2little(*(unsigned int *)value);
        uart_send_string("ramfs_end: ");
        uart_hex((unsigned long)ramfs_end);
        uart_send_string("\n");
    }
}

void initrd_exec_prog(char* target) {

    char* target_addr = (char*) 0x20000;

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
            for (unsigned int i = 0; i < filesize; i++){
                *target_addr++ = filedata[i];
            }
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
    
    unsigned long spsr_el1 = 0x3c0;
    unsigned long elr_el1 = 0x20000;
    unsigned long sp = (unsigned long)kmalloc(4096) + 4096;

    // "r": Any general-purpose register, except sp.
    asm volatile("msr spsr_el1, %0" : : "r" (spsr_el1));
    asm volatile("msr elr_el1, %0" : : "r" (elr_el1));
    asm volatile("msr sp_el0, %0" : : "r" (sp));
    asm volatile("eret");

}