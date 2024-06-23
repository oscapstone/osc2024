#include "cpio.h"
#include "io.h"
#include "string.h"
#include "lib.h"                
#include "type.h"
#include "mem.h"

extern void from_el1_to_el0(uint64_t, uint64_t);

#ifndef QEMU
uint64_t CPIO_START_ADDR_FROM_DT = 0;
uint64_t CPIO_END_ADDR_FROM_DT = 0;
#endif

void cpio_list(int argc, char **argv)
{   
#ifndef QEMU
    cpio_newc_header* head = (void*)(uint64_t)CPIO_START_ADDR_FROM_DT;
#else
    cpio_newc_header* head = (void*)(uint64_t)CPIO_ADDR;
#endif

    uint32_t head_size = sizeof(cpio_newc_header);

    while(1)
    {
        /* strtol: string to long (uint32_t)
           (char *str, int base, int size)
        */
        int namesize = strtol(head->c_namesize, 16, 8);
        int filesize = strtol(head->c_filesize, 16, 8);

        // char *filename = (void*)head + head_size;
        char filename[256];
        strncpy(filename, (void*)head + head_size, namesize);
        /*  The above statement equal to the following:
            char filename[256];
            strncpy(filename, (void*)head + head_size, namesize);

            In New ASCII format, the filename is followed by the header
        */
        
        if(strcmp(filename, "TRAILER!!!") == 0) break;
        
        printf("\r\n");
        printf(filename);

        uint32_t offset = head_size + namesize;
        if(offset % 4 != 0) offset = ((offset/4 +1)*4);
        if(filesize % 4 != 0) filesize = (filesize/4 +1)*4;
        /* New ASCII format
            header size + filename size must be multiple of 4
            filesize must be multiple of 4
        */

        head = (void*)head + offset + filesize;
        /*
            Why (void*)head instead of head?
            In C, when you add an integer to a pointer, 
            in this case, 
            head = head + (offset + filesize)*sizeof(cpio_newc_header) in practical.
            But (offset + filesize) is the byte size, not the number of cpio_newc_header.
            So, we need to cast head to void* to make it byte size.
        */
    }
}

void cpio_cat(int argc, char **argv)
{
    // printf("\nFilename: ");
    // char input_filename[256];
    // readcmd(input_filename);
    if(argc < 2){
        printf("\nUsage: cat <filename>");
        return;
    }
    char *input_filename = argv[1];

#ifndef QEMU
    cpio_newc_header* head = (void*)(uint64_t)CPIO_START_ADDR_FROM_DT;
#else
    cpio_newc_header* head = (void*)(uint64_t)CPIO_ADDR;
#endif

    uint32_t head_size = sizeof(cpio_newc_header);

    while(1)
    {
        int namesize = strtol(head->c_namesize, 16, 8);
        int filesize = strtol(head->c_filesize, 16, 8);

        char *filename = (void*)head + head_size;

        uint32_t offset = head_size + namesize;
        if(offset % 4 != 0) offset = ((offset/4 +1)*4);

        if(strcmp(filename, "TRAILER!!!") == 0){
            printf("\nFile not found");
            break;
        }
        else if(strcmp(filename, input_filename) == 0){
            /* The filedata is appended after filename */
            char *filedata = (void*)head + offset;
            printf("\n");
            for(int i=0; i<filesize; i++){
                printfc(filedata[i]);
            }
            break;
        }

        if(filesize % 4 != 0) filesize = (filesize/4 +1)*4;
        head = (void*)head + offset + filesize;
    }

}

void cpio_exec(int argc, char **argv)
{
    if(argc < 2){
        printf("\nUsage: exec <filename>");
        return;
    }

    char *input_filename = argv[1];

#ifndef QEMU
    cpio_newc_header* head = (void*)(uint64_t)CPIO_START_ADDR_FROM_DT;
#else
    cpio_newc_header* head = (void*)(uint64_t)CPIO_ADDR;
#endif

    uint32_t head_size = sizeof(cpio_newc_header);
    while(1)
    {
        int namesize = strtol(head->c_namesize, 16, 8);
        int filesize = strtol(head->c_filesize, 16, 8);

        char *filename = (void*)head + head_size;

        uint32_t offset = head_size + namesize;
        if(offset % 4 != 0) offset = ((offset/4 +1)*4);

        if(strcmp(filename, "TRAILER!!!") == 0){
            printf("\nFile not found");
            break;
        }
        else if(strcmp(filename, input_filename) == 0){
            /* The filedata is appended after filename */
            char *filedata = (void*)head + offset;
            char *user_program_addr = (void*)(uint64_t)USER_START_ADDR;
            for(int i=0; i<filesize; i++){
                *user_program_addr = filedata[i];
                user_program_addr++;
            }
            from_el1_to_el0(USER_START_ADDR, USER_PROCESS_SP);
            return;
        }

        if(filesize % 4 != 0) filesize = (filesize/4 +1)*4;
        head = (void*)head + offset + filesize;
    }

}

int cpio_newc_parser(cpio_newc_header** head, char** pathname, char** filedata)
{
    int ret = 1;
    int namesize = strtol((*head)->c_namesize, 16, 8);
    int filesize = strtol((*head)->c_filesize, 16, 8);
    // *c_mode = (*head)->c_mode;

    uint32_t head_size = sizeof(cpio_newc_header);

    char *filename = (void*)(*head) + head_size;
    *pathname = filename;

    uint32_t offset = head_size + namesize;
    if(offset % 4 != 0) offset = ((offset/4 +1)*4);

    if(strcmp(filename, "TRAILER!!!") == 0){
        // do nothing
    }
    else{
        /* The filedata is appended after filename */
        *filedata = (void*)(*head) + offset;
        ret = 0;
    }

    if(filesize % 4 != 0) filesize = (filesize/4 +1)*4;
    *head = (void*)(*head) + offset + filesize;
    return ret;
}

#ifndef QEMU
void initramfs_callback(char* node_name, char* property_name, fdt_prop* prop)
{
    // reference: https://stackoverflow.com/questions/73974443/how-does-the-linux-kernel-know-about-the-initrd-when-booting-with-a-device-tree
    if((strcmp(node_name, "chosen") == 0) && (strcmp(property_name, "linux,initrd-start") == 0)){
        void *data = (void*)prop + sizeof(fdt_prop);
        CPIO_START_ADDR_FROM_DT = endian_swap(*(uint32_t*)data);
        printf("\nInitramfs Start Address from DT: ");
        printf_hex(CPIO_START_ADDR_FROM_DT);
    }
    if((strcmp(node_name, "chosen") == 0) && (strcmp(property_name, "linux,initrd-end") == 0)){
        void *data = (void*)prop + sizeof(fdt_prop);
        CPIO_END_ADDR_FROM_DT = endian_swap(*(uint32_t*)data);
        printf("\nInitramfs End Address from DT: ");
        printf_hex(CPIO_END_ADDR_FROM_DT);
    }
}
#endif