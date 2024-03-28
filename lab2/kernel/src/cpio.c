#include <stdint.h>
#include "cpio.h"
#include "mini_uart.h"
#include "helper.h"

extern char* _cpio_file;
char buff[1024];

uint8_t hex_char_to_bin(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0; // Not a valid hexadecimal character
}

uint64_t hex_to_bin(const char* hex) {
    uint64_t result = 0;
	for(int i = 0; i < 8; i ++) {
        result = (result << 4) | hex_char_to_bin(hex[i]);
    }
    return result;
}

void parse_cpio_ls() {
	char* cpio_start = _cpio_file;
    struct cpio_newc_header* header = (struct cpio_newc_header*)cpio_start;
    substr(buff, header -> c_magic, 0, 5);
	while (same(buff, "070701")) {

        // Convert c_namesize and c_filesize from ASCII hex to binary
        uint64_t namesize = hex_to_bin(header->c_namesize);
        uint64_t filesize = hex_to_bin(header->c_filesize);

        // Calculate the start of the file name and file data
        char* filename = (char*)(header + 1);
        char* filedata = filename + namesize;
        filedata = (char*)((uintptr_t)filedata + ((4 - ((uintptr_t)filedata & 3)) & 3)); // Align to next 4-byte boundary
	
		substr(buff, filename, 0, namesize - 1);
        if (same(buff, "TRAILER!!!")) {
            break; // End of archive
        }
		output(buff);	

        // Move to the next header, aligning as necessary
        header = (struct cpio_newc_header*)(filedata + filesize);
        header = (struct cpio_newc_header*)((uintptr_t)header + ((4 - ((uintptr_t)header & 3)) & 3)); // Align to next 4-byte boundary
    	substr(buff, header -> c_magic, 0, 5);
    }
}

void parse_cpio_cat(char* name) {
	
	
	char* cpio_start = _cpio_file;
    struct cpio_newc_header* header = (struct cpio_newc_header*)cpio_start;
    substr(buff, header -> c_magic, 0, 5);
	while (same(buff, "070701")) {

        // Convert c_namesize and c_filesize from ASCII hex to binary
        uint64_t namesize = hex_to_bin(header->c_namesize);
        uint64_t filesize = hex_to_bin(header->c_filesize);

        // Calculate the start of the file name and file data
        char* filename = (char*)(header + 1);
        char* filedata = filename + namesize;
        filedata = (char*)((uintptr_t)filedata + ((4 - ((uintptr_t)filedata & 3)) & 3)); // Align to next 4-byte boundary
	
		substr(buff, filename, 0, namesize - 1);
        if (same(buff, "TRAILER!!!")) {
            break; // End of archive
        }

		if (same(buff, name)) {
			substr(buff, filedata, 0, filesize - 1);
			output(buff);
		}
		substr(buff, filename, 0, namesize - 1);

        // Move to the next header, aligning as necessary
        header = (struct cpio_newc_header*)(filedata + filesize);
        header = (struct cpio_newc_header*)((uintptr_t)header + ((4 - ((uintptr_t)header & 3)) & 3)); // Align to next 4-byte boundary
    	substr(buff, header -> c_magic, 0, 5);
    }
}

