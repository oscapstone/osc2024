#include "headers/utils.h"
#include "headers/cpio.h"
#include "headers/uart.h"

void cpio_ls()
{
    char* addr = (char*) cpio_addr;
    while(1)
    {
        struct cpio_header* header = (struct cpio_header*) addr;
        unsigned int filename_size = atoi(header->c_namesize, sizeof(header->c_namesize)); 
        unsigned int content_size = atoi(header->c_filesize, sizeof(header->c_filesize));
        unsigned int header_and_filename_size = sizeof(struct cpio_header) + filename_size;

        //alignment
        // header_and_filename_size = header_and_filename_size%4==0 ? header_and_filename_size : header_and_filename_size + 4 - header_and_filename_size%4;
        // content_size = content_size%4==0 ? content_size : content_size + 4 - content_size % 4;

        align(&header_and_filename_size, 4);
        align(&content_size, 4);

        char *filename = (char*) (addr + sizeof(struct cpio_header));

        if(strcmp(filename, END_TRAIL)) break;

        display(filename);
        display(" ");

        addr += (header_and_filename_size + content_size);
    }
    display("\n");
}

void cpio_cat(char *filename)
{
    char *addr = (char*) cpio_addr;
    while(1)
    {
        struct cpio_header* header = (struct cpio_header*) addr;
        unsigned int filename_size = atoi(header->c_namesize, sizeof(header->c_namesize)); 
        unsigned int content_size = atoi(header->c_filesize, sizeof(header->c_filesize));
        unsigned int header_and_filename_size = sizeof(struct cpio_header) + filename_size;

        align(&header_and_filename_size, 4); 
        align(&content_size, 4);
        
        char *filename_ = (char*) (addr+sizeof(struct cpio_header));
        if(strcmp(filename, filename_))
        {
            char *content = (char*) (filename_ + filename_size);
            for(unsigned int i=0 ; i<content_size ; i++)
                send(content[i]);
            display("\r\n");
            return;
        }
        else if(strcmp(filename_, END_TRAIL)) break;

        addr += (header_and_filename_size + content_size);
    }

    display("File not found.\n");
}