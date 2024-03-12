#include "headers/simple_shell.h"
#include "headers/mini_uart.h"
#include "headers/string.h"

#define BUFFER_SIZE (256)

// asm call
// start pointer, size in 8 bytes alignment
extern void mem_zero( unsigned long start, unsigned long size);

static void read_command( char *buffer, int size);
static void parse_command( char *buffer, int size);
static void help();
static void hello();
static void reboot();

void simple_shell()
{
    mini_uart_init();

    char buffer[ BUFFER_SIZE];
    while ( 1)
    {
        read_command( buffer, sizeof(buffer));
        parse_command( buffer, sizeof(buffer));
    }// while
    return;
}

static void read_command( char *buffer, int size)
{
    mini_uart_puts("# ");
    mem_zero((unsigned long) buffer, size / 8);
    char temp;
    int full = 0;
    // keep the last one '\0'
    for ( int i = 0; i < size - 1; i += 1)
    {
        temp = mini_uart_getc();
        mini_uart_putc( temp);
        buffer[ i] = temp;
        if ( temp == '\n' || temp == '\r')
        {
            // finished line
            break;
        }// if
        
        if ( i == size - 2)
        {
            full = 1;
        }// if
    }// for i

    if ( full)
    {
        mini_uart_puts("\n{Buffer full: aborting}\n");
    }// if
    
    return;
}

typedef struct
{
    char *name;
    void (*action)();
    char *description;
} s_command;

static s_command valid_commands[] =
{
    (s_command){ "help", help, "print this help menu"},
    (s_command){ "hello", hello, "print Hello World!"},
    (s_command){ "reboot", reboot, "reboot the device"}
};

static int valid_command_size = sizeof( valid_commands) / sizeof( valid_commands[ 0]);

static void parse_command( char *buffer, int size)
{
    for ( int i = 0; i < valid_command_size; i += 1)
    {
        if ( strcmp( buffer, valid_commands[ i].name) == 0)
        {
            return valid_commands[ i].action();
        }// if
    }// for i
    
    return;
}

static void help()
{
    for ( int i = 0; i < valid_command_size; i += 1)
    {
        mini_uart_puts( valid_commands[ i].name);
        mini_uart_puts("\t : ");
        mini_uart_puts( valid_commands[ i].description);
        mini_uart_puts("\n");
    }// for i

    return;
}

static void hello()
{
    mini_uart_puts("Hello World!\n");
    return;
}

static void reboot()
{
    mini_uart_puts("reboot not implemented\n");
    return;
}
