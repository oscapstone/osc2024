#include "../include/shell.h"
#include "../include/power.h"
#include "../include/uart1.h"
#include "../include/utils.h"

extern char *_dtb;
extern char _start[];

struct CLI_CMDS cmd_list[CLI_MAX_CMD] = {
	/* data */
	{.command = "loadimg", .help = "load image via uart1"},
	{.command = "help", .help = "print all available commands"},
	{.command = "reboot", .help = "reboot the device"}};

/* Overwite image file into _start */
void do_cmd_loadimg()
{
	char *bak_dtb = _dtb;
	char c;
	unsigned long long kernel_size = 0;
	char *kernel_start = (char *)(&_start);

	uart_puts("Please upload the image file.\r\n");

	for (int i = 0; i < 8; i++)
	{ // get the size of kernel(long long int -> 64 bits, binary)
		c = uart_getc();
		kernel_size += c << (i * 8);
	}
	for (int i = 0; i < kernel_size; i++)
	{ // load kernel
		c = uart_getc();
		kernel_start[i] = c;
	}
	uart_puts("Image file downloaded successfully.\r\n");
	uart_puts("Point to new kernel ...\r\n");

	// it's a function call, calling the function located at kernel_start with param "bak_dtb"
	((void (*)(char *))kernel_start)(bak_dtb);
}

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
	if (cli_cmd_strcmp(buffer, "loadimg") == 0)
	{
		do_cmd_loadimg();
	}
	else if (cli_cmd_strcmp(buffer, "help") == 0)
	{
		do_cmd_help();
	}
	else if (cli_cmd_strcmp(buffer, "reboot") == 0)
	{
		do_cmd_reboot();
	}
	else if (*buffer)
	{
		uart_puts(buffer);
		uart_puts(": command not found\r\n");
	}
}

void cli_print_banner()
{
	uart_puts("\n=======================================\r\n");
	uart_puts("  Welcome to NYCU-OSC 2024 Lab2 Shell  \r\n");
	uart_puts("=======================================\r\n");
}

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

void do_cmd_reboot()
{
	uart_puts("Reboot in 5 seconds ...\r\n\r\n");
	volatile unsigned int *rst_addr = (unsigned int *)PM_RSTC;
	*rst_addr = PM_PASSWORD | 0x20;
	volatile unsigned int *wdg_addr = (unsigned int *)PM_WDOG;
	*wdg_addr = PM_PASSWORD | 5;
}