#include "cpio.h"
#include "string.h"
#include "uart.h"

static int hex_to_int(char *p, int len)
{
	int val = 0;
	int tmp;

	for (int i = 0; i < len; i++) {
		tmp = *(p + i);
		if (tmp >= 'A')
			tmp = tmp - 'A' + 10;
		else
			tmp -= '0';
		val <<= 4;
		val |= tmp;
	}
	return val;
}

// For cpio alignment issue: https://stackoverflow.com/questions/59138551/extra-bytes-are-padding-in-the-generated-file-from-cpio

void cpio_ls()
{
	struct cpio_newc_header *fs = (struct cpio_newc_header *)0x20000000;
	char *current = (char *)0x20000000;

	while (1) {
		fs = (struct cpio_newc_header *)current;
		int name_size = hex_to_int(fs->c_namesize, 8);
		int file_size = hex_to_int(fs->c_filesize, 8);

		current += sizeof(*fs);
		if (strcmp(current, "TRAILER!!!") == 0)
			break;

		uart_puts(current);
		uart_puts("\r\n");
		current += name_size;
		if ((current - (char *)fs) % 4 != 0)
			current += (4 - (current - (char *)fs) % 4);

		current += file_size;
		if ((current - (char *)fs) % 4 != 0)
			current += (4 - (current - (char *)fs) % 4);
	}
}

void cpio_cat()
{
	char ch;
	char filename[1024];
	int idx = 0;

	uart_puts("\r\n");
	uart_puts("filename: ");
	while (1) {
		ch = uart_getc();
		uart_send(ch);
		if (ch == '\n') {
			filename[idx] = '\0';
			idx = 0;
			break;
		} else {
			filename[idx] = ch;
			idx++;
		}
	}

	struct cpio_newc_header *fs = (struct cpio_newc_header *)0x20000000;
	char *current = (char *)0x20000000;
	while (1) {
		fs = (struct cpio_newc_header *)current;
		int name_size = hex_to_int(fs->c_namesize, 8);
		int file_size = hex_to_int(fs->c_filesize, 8);
		current += sizeof(*fs);
		if (strcmp(current, "TRAILER!!!") == 0) {
			uart_puts("\r\n");
			uart_puts(filename);
			uart_puts(": No such file.\n");
			break;
		}
		if (strcmp(current, filename) == 0) {
			current += name_size;
			if ((current - (char *)fs) % 4 != 0)
				current += (4 - (current - (char *)fs) % 4);
			// uart_puts(current);
            for (int i = 0; i < file_size; i++)
                uart_send(current[i]);
			uart_puts("\r\n");
			break;
		}
		current += name_size;
		if ((current - (char *)fs) % 4 != 0)
			current += (4 - (current - (char *)fs) % 4);

		current += file_size;
		if ((current - (char *)fs) % 4 != 0)
			current += (4 - (current - (char *)fs) % 4);
	}
}
