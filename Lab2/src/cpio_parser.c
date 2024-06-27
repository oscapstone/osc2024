#include "../include/uart.h"
#include "../include/cpio_parser.h"
#include "../include/my_stddef.h"
#include "../include/my_string.h"
#include "../include/my_stdlib.h"
#include "../include/my_stdint.h"

void cpio_ls(char *cpio_archive_start) {
    char *ptr = cpio_archive_start;

    while (1) {
        struct cpio_newc_header *header = (struct cpio_newc_header *)ptr;


        // Safety check: Ensure ptr does not go beyond the end of the archive
        if (strncmp(header->c_magic, CPIO_NEWC_MAGIC, 6) != 0) {
            uart_puts("Invalid CPIO magic number\n");
            break;
        }

        //The  "new"  ASCII format	uses 8-byte hexadecimal	fields for all numbers
        uint64_t namesize = hex_to_uint(header->c_namesize);
        uint64_t filesize = hex_to_uint(header->c_filesize);

        /*Each  file  system  object  in a	cpio archive comprises a header	record
        with basic numeric metadata followed by the full pathname of the entry and the file data.*/
        ptr += HEADER_SIZE;
        char *filename = ptr;
        ptr += namesize;

        //allign to 4 byte
        //the  filedata is padded to a multiple of four bytes..
        ptr = alignas(4, ptr);

        if (strcmp(filename, "TRAILER!!!") == 0) {
            break;
        }

        // Safety check: Ensure content does not go beyond the end of the archive
        if (ptr + filesize - cpio_archive_start >= MAX_ARCHIVE_SIZE) {
            uart_puts("Error: File content exceeds archive size\n");
            break;
        }

        ptr += filesize;

        //allign to 4 byte
        ptr = alignas(4, ptr);


        uart_puts(filename);
        uart_puts("\n");
        
    }
}

void cpio_cat(char *cpio_archive_start, const char *filename) {
    char *ptr = cpio_archive_start;
    int file_exist=0;
    while (1) {
        struct cpio_newc_header *header = (struct cpio_newc_header *)ptr;
        
        // Safety check: Ensure ptr does not go beyond the end of the archive
        if (strncmp(header->c_magic, CPIO_NEWC_MAGIC, 6) != 0) {
            uart_puts("Invalid CPIO magic number\n");
            return;
        }

        uint64_t namesize = hex_to_uint(header->c_namesize);
        uint64_t filesize = hex_to_uint(header->c_filesize);

        ptr += HEADER_SIZE;
        char *current_filename = ptr;
        ptr += namesize;

        // Align to 4 bytes
        ptr = alignas(4, ptr);

        // Safety check: Ensure content does not go beyond the end of the archive
        if (ptr + filesize - cpio_archive_start >= MAX_ARCHIVE_SIZE) {
            uart_puts("Error: File content exceeds archive size\n");
            break;
        }


        char *content = ptr;
        ptr += filesize;

        //allign to 4 byte
        ptr = alignas(4, ptr);

        if (strcmp(current_filename, filename) == 0) {
            // Print file content
            uart_puts(content);
            uart_puts("\n");
            return;
        }

        // Check if we've reached the end of the archive
        if (strcmp(current_filename, "TRAILER!!!") == 0) {
            uart_puts("File not found\n");
            uart_puts("\n");
            return;
        }
    }
    if (file_exist==0){
        uart_puts("Can't find :");
        uart_puts(filename);
        uart_puts("Please check if the file name is correct!");
    }

}