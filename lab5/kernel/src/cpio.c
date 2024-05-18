#include <stdint.h>
#include <stddef.h>
#include "cpio.h"
#include "mini_uart.h"
#include "helper.h"
#include "alloc.h"

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

void* cpio_find(char* name) {
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
			return filedata;
		}
		substr(buff, filename, 0, namesize - 1);

        // Move to the next header, aligning as necessary
        header = (struct cpio_newc_header*)(filedata + filesize);
        header = (struct cpio_newc_header*)((uintptr_t)header + ((4 - ((uintptr_t)header & 3)) & 3)); // Align to next 4-byte boundary
    	substr(buff, header -> c_magic, 0, 5);
    }
	return 0;
}

void cpio_parse_ls() {
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
		uart_printf("%s\r\n", buff);

        // Move to the next header, aligning as necessary
        header = (struct cpio_newc_header*)(filedata + filesize);
        header = (struct cpio_newc_header*)((uintptr_t)header + ((4 - ((uintptr_t)header & 3)) & 3)); // Align to next 4-byte boundary
    	substr(buff, header -> c_magic, 0, 5);
    }
}

void cpio_parse_cat(char* name) {
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
			uart_printf("%s\r\n", buff);
		}
		substr(buff, filename, 0, namesize - 1);

        // Move to the next header, aligning as necessary
        header = (struct cpio_newc_header*)(filedata + filesize);
        header = (struct cpio_newc_header*)((uintptr_t)header + ((4 - ((uintptr_t)header & 3)) & 3)); // Align to next 4-byte boundary
    	substr(buff, header -> c_magic, 0, 5);
    }
}

void cpio_load(char* str) {
	void* pos = cpio_find(str); 
	void* stack = my_malloc(4096);
	stack += 4096 - 1;
	void* code = my_malloc(4096);
	strcpy(pos, code, 4096);
	
	uart_printf("Running code from %x...\n", pos);
	asm volatile(
		"mov x1, 0;"
		"msr spsr_el1, x1;"
		"mov x1, %[code];"
		"mov x2, %[sp];"
		"msr elr_el1, x1;"
		"msr sp_el0, x2;"
		"eret;"
		:
		: [code] "r" (code), [sp] "r" (stack)
		: "x1", "x2"
	);
}

char* get_cpio_end() {
	char* cpio_start = _cpio_file;
    struct cpio_newc_header* header = (struct cpio_newc_header*)cpio_start;
    substr(buff, header -> c_magic, 0, 5);
	while (same(buff, "070701")) {
        uint64_t namesize = hex_to_bin(header->c_namesize);
        uint64_t filesize = hex_to_bin(header->c_filesize);

        char* filename = (char*)(header + 1);
        char* filedata = filename + namesize;
        filedata = (char*)((uintptr_t)filedata + ((4 - ((uintptr_t)filedata & 3)) & 3)); // Align to next 4-byte boundary
	
		substr(buff, filename, 0, namesize - 1);
        if (same(buff, "TRAILER!!!")) {
        	return filedata + filesize;
		}

		substr(buff, filename, 0, namesize - 1);

        // Move to the next header, aligning as necessary
        header = (struct cpio_newc_header*)(filedata + filesize);
        header = (struct cpio_newc_header*)((uintptr_t)header + ((4 - ((uintptr_t)header & 3)) & 3)); // Align to next 4-byte boundary
    	substr(buff, header -> c_magic, 0, 5);
    }
	return header;
}
