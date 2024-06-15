#include <stdint.h>
#include <stddef.h>
#include "cpio.h"
#include "mini_uart.h"
#include "helper.h"
#include "alloc.h"
#include "thread.h"
#include "mmu.h"
#include "utils.h"

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
			uart_printf ("Cat %d\r\n", filesize);
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

extern thread* get_current();

void cpio_load(char* str) {
	irq(0);
	thread_init();
	thread* cur = get_current();
	uart_printf ("cur: %llx\r\n", cur);
	long xxx = read_sysreg(tpidr_el1);
	uart_printf ("xxx: %llx\r\n", xxx);

	void* pos = cpio_find(str); 
	cur -> code = my_malloc(4096 * 256);
	cur -> code_size = 4096 * 256;
	for (int i = 0; i < cur -> code_size; i ++) {
		((char*)cur -> code)[i] = ((char*)pos)[i];
	}

	uart_printf ("%llx, %llx, %llx\r\n", cur -> PGD, pa2va(cur -> PGD), &(cur -> PGD));

	setup_peripheral_identity(pa2va(cur -> PGD));
	for (int i = 0; i < cur -> code_size; i += 4096) {
		map_page(pa2va(cur -> PGD), i, va2pa(cur -> code) + i, (1 << 6));
	}
	for (int i = 0; i < 4; i ++) {
		map_page (pa2va(cur -> PGD), 0xffffffffb000L + i * 4096, va2pa(cur -> stack_start) + i * 4096, (1 << 6));
	}
	
	uart_printf ("%llx\r\n", cur -> PGD);
	asm volatile("dsb ish");
	write_sysreg(ttbr0_el1, cur -> PGD);
	asm volatile("dsb ish");
    asm volatile("tlbi vmalle1is");
    asm volatile("dsb ish");
    asm volatile("isb");
	
	cur -> code = 0;
	cur -> stack_start = 0xffffffffb000L;
	
	long ttbr1 = read_sysreg(ttbr1_el1);
	long ttbr0 = read_sysreg(ttbr0_el1);
	uart_printf ("TTBR0: %llx, TTBR1: %llx\r\n", ttbr0, ttbr1);

	uart_printf ("cur -> PGD %llx\r\n", cur -> PGD);

	// uart_printf("Running code from %llx, which is %llx\n", cur -> code, trans(cur -> code));


	uart_printf ("uart printf: %llx\r\n", uart_printf);

	uart_printf("Running code from %llx, which is %llx\n", cur -> code, trans_el0(cur -> code));
	ttbr1 = read_sysreg(ttbr1_el1);
	ttbr0 = read_sysreg(ttbr0_el1);
	uart_printf ("TTBR0: %llx, TTBR1: %llx, &TTBR0: %llx\r\n", ttbr0, ttbr1, &ttbr0);

	write_sysreg(spsr_el1, 0);	
	write_sysreg(elr_el1, cur -> code);
	write_sysreg(sp_el0, 0xfffffffff000L);
	uart_printf ("sp %llx\r\n",cur -> sp);
	asm volatile (
		"mov sp, %0;"
		// "msr DAIFclr, 0xf;"
		"eret;"
		:
		: "r" (cur -> sp) 
	);
	long tt = read_sysreg(elr_el1);
	uart_printf ("should go to %llx, but instead %llx\r\n", 0, tt);
	while (1);
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
