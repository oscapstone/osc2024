#include "../include/shell.h"
#include "../include/dtb.h"
#include "../include/mbox.h"
#include "../include/power.h"
#include "../include/uart1.h"
#include "../include/utils.h"
#include "../include/heap.h"
#include "../include/cpio.h"

extern char *dtb_ptr;
void *CPIO_DEFAULT_PLACE; // root of ramfs

struct CLI_CMDS cmd_list[CLI_MAX_CMD] = {{.command = "cat", .help = "concatenate files and print on the standard output"},
										 {.command = "dtb", .help = "show device tree"},
										 {.command = "hello", .help = "print Hello World!"},
										 {.command = "help", .help = "print all available commands"},
										 {.command = "malloc", .help = "simple allocator in heap session"},
										 {.command = "info", .help = "get device information via mailbox"},
										 {.command = "ls", .help = "list directory contents"},
										 {.command = "reboot", .help = "reboot the device"}};

int cli_cmd_strcmp(const char *p1, const char *p2)
{
	const unsigned char *s1 = (const unsigned char *)p1;
	const unsigned char *s2 = (const unsigned char *)p2;
	unsigned char c1, c2;

	do
	{
		c1 = (unsigned char)*s1++;
		c2 = (unsigned char)*s2++;
		if (c1 == '\0')
			return c1 - c2;
	} while (c1 == c2);
	return c1 - c2;
}

void cli_cmd_clear(char *buffer, int length)
{
	for (int i = 0; i < length; i++)
	{
		buffer[i] = '\0';
	}
};

void cli_cmd_read(char *buffer)
{
	char c = '\0';
	int idx = 0;
	while (1)
	{
		if (idx >= CMD_MAX_LEN)
			break;

		c = uart_recv();
		if (c == '\n')
		{
			uart_puts("\r\n");
			break;
		}
		if (c > 16 && c < 32)
			continue;
		if (c > 127)
			continue;
		buffer[idx++] = c;
		uart_send(c);
	}
}

void cli_cmd_exec(char *buffer)
{
	if (!buffer)
		return;

	char *cmd = buffer;
	char *argvs; // get the first param after cmd

	while (1)
	{
		if (*buffer == '\0')
		{
			argvs = buffer;
			break;
		}
		if (*buffer == ' ')
		{
			*buffer = '\0';
			argvs = buffer + 1;
			break;
		}
		buffer++;
	}

	if (strcmp(cmd, "cat") == 0)
	{
		do_cmd_cat(argvs);
	}
	else if (strcmp(cmd, "dtb") == 0)
	{
		do_cmd_dtb();
	}
	else if (strcmp(cmd, "hello") == 0)
	{
		do_cmd_hello();
	}
	else if (strcmp(cmd, "help") == 0)
	{
		do_cmd_help();
	}
	else if (strcmp(cmd, "info") == 0)
	{
		do_cmd_info();
	}
	else if (strcmp(cmd, "malloc") == 0)
	{
		do_cmd_malloc();
	}
	else if (strcmp(cmd, "ls") == 0)
	{
		do_cmd_ls(argvs);
	}
	else if (strcmp(cmd, "reboot") == 0)
	{
		do_cmd_reboot();
	}
	else
	{
		uart_puts("%s : command not found\r\n", cmd);
	}
}

void cli_print_banner()
{
	uart_puts("=======================================\r\n");
	uart_puts("  Welcome to NYCU-OSC 2024 Lab2 Shell  \r\n");
	uart_puts("=======================================\r\n");
}

void do_cmd_cat(char *filepath)
{
	char *c_filepath;
	char *c_filedata;
	unsigned int c_filesize;
	struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

	// traverse the whole ramdisk, check filename one by one
	while (header_ptr != 0)
	{
		// func return -1 when error
		int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
		if (error)
		{
			uart_puts("cpio parse error");
			break;
		}

		if (strcmp(c_filepath, filepath) == 0)
		{
			uart_puts("%s", c_filedata);
			break;
		}

		// if is TRAILER!!!
		if (header_ptr == 0)
		{
			uart_puts("cat: %s: No such file or directory", filepath);
		}
	}
	uart_puts("\n");
}

void do_cmd_dtb() { traverse_device_tree(dtb_ptr, dtb_callback_show_tree); }

void do_cmd_help()
{
	for (int i = 0; i < CLI_MAX_CMD; i++)
	{
		uart_puts(cmd_list[i].command);
		uart_puts("\t\t: ");
		uart_puts(cmd_list[i].help);
		uart_puts("\r\n");
	}
}

void do_cmd_hello()
{ // hello
	uart_puts("Hello World!\r\n");
}

void do_cmd_info()
{
	// print hw revision
	pt[0] = 8 * 4;
	pt[1] = MBOX_REQUEST_PROCESS;
	pt[2] = MBOX_TAG_GET_BOARD_REVISION;
	pt[3] = 4;
	pt[4] = MBOX_TAG_REQUEST_CODE;
	pt[5] = 0;
	pt[6] = 0;
	pt[7] = MBOX_TAG_LAST_BYTE;

	if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
	{
		uart_puts("Hardware Revision\t: ");
		uart_2hex(pt[6]);
		uart_2hex(pt[5]);
		uart_puts("\r\n");
	}
	// print arm memory
	pt[0] = 8 * 4;
	pt[1] = MBOX_REQUEST_PROCESS;
	pt[2] = MBOX_TAG_GET_ARM_MEMORY;
	pt[3] = 8;
	pt[4] = MBOX_TAG_REQUEST_CODE;
	pt[5] = 0;
	pt[6] = 0;
	pt[7] = MBOX_TAG_LAST_BYTE;

	if (mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt)))
	{
		uart_puts("ARM Memory Base Address\t: ");
		uart_2hex(pt[5]);
		uart_puts("\r\n");
		uart_puts("ARM Memory Size\t\t: ");
		uart_2hex(pt[6]);
		uart_puts("\r\n");
	}
}

void do_cmd_malloc()
{
	char *test1 = malloc(0x18);
	memcpy(test1, "test malloc1", sizeof("test amlloc1"));
	uart_puts("%s\n", test1);

	char *test2 = malloc(0x20);
	memcpy(test2, "test malloc2", sizeof("test amlloc2"));
	uart_puts("%s\n", test2);

	char *test3 = malloc(0x28);
	memcpy(test3, "test malloc3", sizeof("test amlloc3"));
	uart_puts("%s\n", test3);
}

void do_cmd_ls(char *dir)
{
	char *c_filepath;
	char *c_filedata;
	unsigned int c_filesize;
	struct cpio_newc_header *header_ptr = CPIO_DEFAULT_PLACE;

	while (header_ptr != 0)
	{
		int error = cpio_newc_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
		if (error)
		{
			uart_puts("cpio parse error");
			break;
		}

		if (header_ptr != 0)
		{
			uart_puts("%s\n", c_filepath);
		}
	}
}

void do_cmd_reboot()
{
	uart_puts("Reboot in 5 seconds ...\r\n\r\n");
	volatile unsigned int *rst_addr = (unsigned int *)PM_RSTC;
	*rst_addr = PM_PASSWORD | 0x20;
	volatile unsigned int *wdg_addr = (unsigned int *)PM_WDOG;
	*wdg_addr = PM_PASSWORD | 5;
}