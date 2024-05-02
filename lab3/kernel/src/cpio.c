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

struct filesystem {
	char datas[100000];
	char names[10000];
	int names_pos[1024];
	int datas_pos[1024];
	int n;
};

static struct filesystem fs;

void parse_cpio() {
	char* cpio_start = _cpio_file;
    struct cpio_newc_header* header = (struct cpio_newc_header*)cpio_start;
    substr(buff, header -> c_magic, 0, 5);
	int n = 0;
	int name_ind = 0;
	int data_ind = 0;
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

		fs.names_pos[n] = name_ind;
		for (int i = 0; i < namesize; i ++) {
			fs.names[name_ind ++] = filename[i];
		}
		fs.datas_pos[n] = data_ind;
		for (int i = 0; i < filesize; i ++) {
			fs.datas[data_ind ++] = filedata[i];
		}
		n ++;

        // Move to the next header, aligning as necessary
        header = (struct cpio_newc_header*)(filedata + filesize);
        header = (struct cpio_newc_header*)((uintptr_t)header + ((4 - ((uintptr_t)header & 3)) & 3)); // Align to next 4-byte boundary
    	substr(buff, header -> c_magic, 0, 5);
    }
	fs.names_pos[n] = name_ind;
	fs.datas_pos[n] = data_ind;
	fs.n = n;
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

void cpio_ls() {
	uart_printf("There're %d files:\n", fs.n);
	for(int i = 0; i < fs.n; i ++) {
		substr(buff, fs.names, fs.names_pos[i], fs.names_pos[i + 1] - 1);
		uart_printf("%s\r\n", buff);
	}
}

void cpio_cat(char* str) {
	for(int i = 0; i < fs.n; i ++) {
		substr(buff, fs.names, fs.names_pos[i], fs.names_pos[i + 1] - 1);
		if (same(buff, str)) {
			substr(buff, fs.datas, fs.datas_pos[i], fs.datas_pos[i + 1] - 1);
			for(int j = fs.datas_pos[i]; j < fs.datas_pos[i + 1]; j ++) {
				uart_send(fs.datas[j]);
			}
		}
	}
}

void cpio_load(char* str) {
	for(int i = 0; i < fs.n; i ++) {
		substr(buff, fs.names, fs.names_pos[i], fs.names_pos[i + 1] - 1);
		if (same(buff, str)) {
			void* pos = &fs.datas[fs.datas_pos[i]]; 
			uart_printf("Running code from %x...\n", pos);
			asm volatile(
                "mov x1, 0x3c0;"
                "msr spsr_el1, x1;"
                "mov x1, %[var1];"
                "ldr x2, =0x1000000;"
                "msr elr_el1, x1;"
                "msr sp_el0, x2;"
                "eret"
                :
                : [var1] "r" (pos)
                : "x1", "x2"
            );
		}
	}
}
