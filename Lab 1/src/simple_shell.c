#include <stddef.h>
#include "mini_uart.h"
#include "mailbox.h"
#include "power.h"
#include "string.h"


#define BUFFER_MAX_SIZE     256

char *read_command(char *buffer);
void parse_command(char *buffer);

void print_help();

void mailbox_get_board_revision();
void mailbox_get_arm_memory();
void mailbox_get_vc_info();


void simple_shell() 
{

    while (1)
    {
        char buffer[BUFFER_MAX_SIZE];
        read_command(buffer);
        parse_command(buffer);
    }
}


char *read_command(char *buffer)
{
    size_t index = 0;
    char r = 0;
    mini_uart_puts("\r\n");
    mini_uart_puts("# ");
    do {
        r = mini_uart_getc();
        mini_uart_putc(r);
        buffer[index++] = r;
    } while (index < (BUFFER_MAX_SIZE - 1) && r != '\n');
    if (r == '\n') index--;
    buffer[index] = '\0';
    mini_uart_puts("\r\n");
    return buffer;
}


void parse_command(char *buffer)
{
    if (str_eql(buffer, "help")) {
        print_help();
    }

    else if (str_eql(buffer, "hello")) {
        mini_uart_putln("Hello world!");
    }

    else if (str_eql(buffer, "mailbox")) {
        mailbox_get_board_revision();
        mini_uart_puts("\r\n");
        mailbox_get_arm_memory();
        mini_uart_puts("\r\n");
        mailbox_get_vc_info();
    }

    else if (str_eql(buffer, "reboot")) {
        mini_uart_putln("rebooting...");
        reset(1000);
    }

    else {
        mini_uart_puts("not a command: ");
        mini_uart_puts(buffer);
        mini_uart_puts("\r\n");
    }
}


void print_help()
{
    mini_uart_puts("help\t:");
    mini_uart_putln("print this help menu");
    mini_uart_puts("hello\t:");
    mini_uart_putln("print Hello World!");
    mini_uart_puts("mailbox\t:");
    mini_uart_putln("the mailbox hardware info");
    mini_uart_puts("reboot\t:");
    mini_uart_putln("reboot the device");
}



void mailbox_get_board_revision()
{
    mbox[0] = 7 * 4;
    mbox[1] = MBOX_REQUEST;                         
    
    mbox[2] = MBOX_TAG_GETREVISION;
    mbox[3] = 4;                                    
    mbox[4] = MBOX_TAG_REQUEST;
    mbox[5] = 0;

    mbox[6] = MBOX_TAG_END;
    
    mailbox_call(MBOX_CH_PROP);    // message passing procedure call, you should implement it following the 6 steps provided above.
 
    mini_uart_puts("Board revision: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[5]);                         // it should be 0xa020d3 for rpi3 b+
    mini_uart_puts("\r\n");
}

void mailbox_get_arm_memory()
{
    mbox[0] = 8 * 4;
    mbox[1] = MBOX_REQUEST;

    mbox[2] = MBOX_TAG_GETMEMORY;
    mbox[3] = 8;
    mbox[4] = MBOX_TAG_REQUEST;
    mbox[5] = 0;                                    // memory basee address
    mbox[6] = 0;                                    // memory size

    mbox[7] = MBOX_TAG_END;
    
    mailbox_call(MBOX_CH_PROP);
 
    mini_uart_putln("Arm memory info:");

    mini_uart_puts("-> memory base: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[5]);
    mini_uart_puts("\r\n");

    mini_uart_puts("-> memory size: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[6]);
    mini_uart_puts("\r\n");
}


void mailbox_get_vc_info()
{
    char buffer[40];

    /* size & request */
    mbox[0]  = 35 * 4;
    mbox[1]  = MBOX_REQUEST;

    /* tags begin */
    mbox[2]  = MBOX_TAG_GETVCMEM;
    mbox[3]  = 8;
    mbox[4]  = MBOX_TAG_REQUEST;
    mbox[5]  = 0;                               // VC memory base
    mbox[6]  = 0;                               // VC memory size

    mbox[7]  = MBOX_TAG_ALLOCATEFB;             // Allocate frame buffer command
    mbox[8]  = 8;
    mbox[9]  = MBOX_TAG_REQUEST;
    mbox[10] = 4096;                            // frame buffer base address
    mbox[11] = 0;                               // frame buffer size
    
    mbox[12] = MBOX_TAG_GETPHYSICALWH;          
    mbox[13] = 8;
    mbox[14] = MBOX_TAG_REQUEST;
    mbox[15] = 0;                               // display width
    mbox[16] = 0;                               // display height

    mbox[17] = MBOX_TAG_GETVIRTUALWH;          
    mbox[18] = 8;
    mbox[19] = MBOX_TAG_REQUEST;
    mbox[20] = 0;                               // virtual width
    mbox[21] = 0;                               // virtual height

    mbox[22] = MBOX_TAG_GETVIRTUALOFFSET;
    mbox[23] = 8;
    mbox[24] = MBOX_TAG_REQUEST;
    mbox[25] = 0;                               // offset X
    mbox[26] = 0;                               // offset Y

    mbox[27] = MBOX_TAG_GETDEPTH;
    mbox[28] = 4;
    mbox[29] = MBOX_TAG_REQUEST;
    mbox[30] = 0;                               // depth

    mbox[31] = MBOX_TAG_GETPIXELORDER;
    mbox[32] = 4;
    mbox[33] = MBOX_TAG_REQUEST;
    mbox[34] = 0;                               // 0x0: BGR, 0x1: RGB

    mbox[35] = MBOX_TAG_GETPITCH;
    mbox[36] = 4;
    mbox[37] = MBOX_TAG_REQUEST;
    mbox[38] = 0;                               // pitch

    /* tags end */
    mbox[39] = MBOX_TAG_END;
    
    mailbox_call(MBOX_CH_PROP); // message passing procedure call, you should implement it following the 6 steps provided above.
 
    mini_uart_putln("VideoCore info:");
    
    mini_uart_puts("-> memory base: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[5]);
    mini_uart_puts("\r\n");

    mini_uart_puts("-> memory size: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[6]);
    mini_uart_puts("\r\n"); 
     
    mini_uart_puts("-> frame buffer base: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[10] & 0x3fffffff);
    mini_uart_puts("\r\n");

    mini_uart_puts("-> frame buffer size: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[11]);
    mini_uart_puts("\r\n"); 

    mini_uart_puts("-> physical width: \t");
    uint_to_ascii(mbox[15], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n");

    mini_uart_puts("-> physical height: \t");
    uint_to_ascii(mbox[16], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n"); 
    
    mini_uart_puts("-> virtual width: \t");
    uint_to_ascii(mbox[20], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n");

    mini_uart_puts("-> virtual height: \t");
    uint_to_ascii(mbox[21], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n"); 
    
    mini_uart_puts("-> buffer offset X: \t");
    uint_to_ascii(mbox[25], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n");

    mini_uart_puts("-> buffer offset Y: \t");
    uint_to_ascii(mbox[26], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n"); 
    
    mini_uart_puts("-> depth: \t\t");
    uint_to_ascii(mbox[30], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n");
    
    mini_uart_puts("-> pixel order: \t");
    if (mbox[34] == 0x0) mini_uart_putln("BGR"); else mini_uart_putln("RGB");

    mini_uart_puts("-> pitch: \t\t");
    uint_to_ascii(mbox[38], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n");
}


