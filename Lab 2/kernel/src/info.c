#include "mini_uart.h"
#include "mailbox.h"
#include "string.h"


void 
info_board_revision()
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
    mini_uart_endl();
}


void 
info_memory()
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

    mini_uart_puts(" > memory base: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[5]);
    mini_uart_endl();

    mini_uart_puts(" > memory size: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[6]);
    mini_uart_endl();
}


void 
info_videocore()
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
    mbox[10] = 4096;                            // alignment in bytes   ==> frame buffer base address
    mbox[11] = 0;                               //                      ==> frame buffer size
    
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
    
    mini_uart_puts(" > memory base: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[5]);
    mini_uart_endl();

    mini_uart_puts(" > memory size: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[6]);
    mini_uart_endl();
     
    mini_uart_puts(" > frame buffer base: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[10] & 0x3fffffff);
    mini_uart_puts("\r\n");

    mini_uart_puts(" > frame buffer size: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[11]);
    mini_uart_endl();

    mini_uart_puts(" > physical width: \t");
    uint32_to_ascii(mbox[15], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n");

    mini_uart_puts(" > physical height: \t");
    uint32_to_ascii(mbox[16], buffer);
    mini_uart_puts(buffer);
    mini_uart_endl();
    
    mini_uart_puts(" > virtual width: \t");
    uint32_to_ascii(mbox[20], buffer);
    mini_uart_puts(buffer);
    mini_uart_endl();

    mini_uart_puts(" > virtual height: \t");
    uint32_to_ascii(mbox[21], buffer);
    mini_uart_puts(buffer);
    mini_uart_endl();
    
    mini_uart_puts(" > buffer offset X: \t");
    uint32_to_ascii(mbox[25], buffer);
    mini_uart_puts(buffer);
    mini_uart_endl();

    mini_uart_puts(" > buffer offset Y: \t");
    uint32_to_ascii(mbox[26], buffer);
    mini_uart_puts(buffer);
    mini_uart_endl();
    
    mini_uart_puts(" > depth: \t\t");
    uint32_to_ascii(mbox[30], buffer);
    mini_uart_puts(buffer);
    mini_uart_endl();
    
    mini_uart_puts(" > pixel order: \t");
    if (mbox[34] == 0x0) mini_uart_putln("BGR"); else mini_uart_putln("RGB");

    mini_uart_puts(" > pitch: \t\t");
    uint32_to_ascii(mbox[38], buffer);
    mini_uart_puts(buffer);
    mini_uart_endl();
}
