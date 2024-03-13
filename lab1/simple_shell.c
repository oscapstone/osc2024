#include "headers/simple_shell.h"
#include "headers/mini_uart.h"
#include "headers/string.h"
#include "headers/mailbox.h"

#define BUFFER_SIZE (256)

// asm call
// start pointer, size in 8 bytes alignment
extern void mem_zero( unsigned long start, unsigned long size);

typedef struct
{
    char *name;
    void ( *action)( void);
    char *description;
} s_command;

static void read_command( char *buffer, int size);
static void parse_command( char *buffer, int size);
static void help( void);
static void hello( void);
static void reboot( void);
static void mailbox_get_info( void);

static const s_command valid_commands[] =
{
    (s_command){ "help", help, "print this help menu"},
    (s_command){ "hello", hello, "print Hello World!"},
    (s_command){ "mailbox", mailbox_get_info, "print system information via mailbox"},
    (s_command){ "reboot", reboot, "reboot the device"}
};

static const int valid_command_size = sizeof( valid_commands) / sizeof( valid_commands[ 0]);

void simple_shell()
{
    mini_uart_init();

    char buffer[ BUFFER_SIZE];
    while ( 1)
    {
        read_command( buffer, sizeof(buffer));
        parse_command( buffer, strlen( buffer));
    }// while
    return;
}

static void read_command( char *buffer, int size)
{
    mini_uart_puts("\r\n# ");
    mem_zero((unsigned long) buffer, size / 8);
    char temp;
    int full = 0;
    // keep the last one '\0'
    for ( int i = 0; i < size - 1; i += 1)
    {
        temp = mini_uart_getc();
        if ( temp == '\n' || temp == '\r')
        {
            // finished line
            // mini_uart_puts("\r\n");
            buffer[ i] = '\0';
            break;
        }// if
        else
        {
            mini_uart_putc( temp);
            buffer[ i] = temp;
        }// else
        
        if ( i == size - 2)
        {
            full = 1;
        }// if
    }// for i

    if ( full)
    {
        mini_uart_puts("\r\n");
        mini_uart_puts("{Buffer full: aborting}");
    }// if
    
    return;
}

static void parse_command( char *buffer, int size)
{
    if ( size == 0)
    {
        return;
    }// if
    
    for ( int i = 0; i < valid_command_size; i += 1)
    {
        if ( strcmp( buffer, valid_commands[ i].name) == 0)
        {
            valid_commands[ i].action();
            break;
        }// if
    }// for i
    
    return;
}

static void help()
{
    for ( int i = 0; i < valid_command_size; i += 1)
    {
        mini_uart_puts("\r\n");
        mini_uart_puts( valid_commands[ i].name);
        mini_uart_puts("\t : ");
        mini_uart_puts( valid_commands[ i].description);
    }// for i

    return;
}

static void hello()
{
    mini_uart_puts("\r\n");
    mini_uart_puts("Hello World!");
    return;
}

static void mailbox_get_info( void)
{
    mailbox_get_board_revision();
    mailbox_get_arm_base_size();
}

static void reboot()
{
    mini_uart_puts("\r\n");
    mini_uart_puts("reboot not implemented");
    return;
}
