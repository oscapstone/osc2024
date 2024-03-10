#include <stdint.h>

#include "initrd.h"
#include "uart.h"
#include "string.h"
#include "utils.h"

static void *ramfs_base = (void *)0x8000000;

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
		else if (*s >= 0)
			r += *s++ - '0';
	}
	return r;
}

void initrd_list()
{
	char *fptr = (char *)ramfs_base;

	// Check if the file is encoded with New ASCII Format
	while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
		cpio_t *header = (cpio_t *)fptr;

		// New ASCII Format uses 8-byte hexadecimal string for all numbers
		int namesize = hextoi(header->c_namesize, 8);
		int filesize = hextoi(header->c_filesize, 8);

		// Total size of (header + pathname) is a multiple of four bytes
		// File data is also padded to a multiple of four bytes
		int headsize = align4(sizeof(cpio_t) + namesize);
		int datasize = align4(filesize);

		// Print file pathname
		char pathname[namesize];
		strncpy(pathname, fptr + sizeof(cpio_t), namesize);
		uart_puts(pathname);
		uart_putc('\n');

		fptr += headsize + datasize;
	}
}

void initrd_cat(const char *target)
{
	char *fptr = (char *)ramfs_base;

	// Check if the file is encoded with New ASCII Format
	while (memcmp(fptr + sizeof(cpio_t), "TRAILER!!!", 10)) {
		cpio_t *header = (cpio_t *)fptr;

		// New ASCII Format uses 8-byte hexadecimal string for all numbers
		int namesize = hextoi(header->c_namesize, 8);
		int filesize = hextoi(header->c_filesize, 8);

		// Total size of (header + pathname) is a multiple of four bytes
		// File data is also padded to a multiple of four bytes
		int headsize = align4(sizeof(cpio_t) + namesize);
		int datasize = align4(filesize);

		// Match target file
		char pathname[namesize];
		strncpy(pathname, fptr + sizeof(cpio_t), namesize);
		if (!strcmp(target, pathname)) {
			// Print its content
			char data[filesize];
			strncpy(data, fptr + headsize, filesize);
			uart_puts(data);
			uart_putc('\n');
			return;
		}

		fptr += headsize + datasize;
	}

	uart_puts("File not found.\n");
}

void initrd_callback(void *addr)
{
	uart_puts("[INFO] Initrd is mounted at ");
	uart_hex((uintptr_t)addr);
	uart_putc('\n');
	ramfs_base = (char *)addr;
}
