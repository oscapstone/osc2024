#include "uart.h"
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
    
    mailbox_call(MBOX_CH_PROP);                     // message passing procedure call
 
    uart_str("Board revision: \t");
    uart_str("0x");
    uart_hex(mbox[5]);                              // it should be 0xa020d3 for rpi3 b+
    uart_endl();
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
 
    uart_line("Arm memory info:");

    uart_str(" > memory base: \t");
    uart_str("0x");
    uart_hex(mbox[5]);
    uart_endl();

    uart_str(" > memory size: \t");
    uart_str("0x");
    uart_hex(mbox[6]);
    uart_endl();
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
 
    uart_line("VideoCore info:");
    
    uart_str(" > memory base: \t");
    uart_str("0x");
    uart_hex(mbox[5]);
    uart_endl();

    uart_str(" > memory size: \t");
    uart_str("0x");
    uart_hex(mbox[6]);
    uart_endl();
     
    uart_str(" > frame buffer base: \t");
    uart_str("0x");
    uart_hex(mbox[10] & 0x3fffffff);
    uart_str("\r\n");

    uart_str(" > frame buffer size: \t");
    uart_str("0x");
    uart_hex(mbox[11]);
    uart_endl();

    uart_str(" > physical width: \t");
    uint32_to_ascii(mbox[15], buffer);
    uart_str(buffer);
    uart_str("\r\n");

    uart_str(" > physical height: \t");
    uint32_to_ascii(mbox[16], buffer);
    uart_str(buffer);
    uart_endl();
    
    uart_str(" > virtual width: \t");
    uint32_to_ascii(mbox[20], buffer);
    uart_str(buffer);
    uart_endl();

    uart_str(" > virtual height: \t");
    uint32_to_ascii(mbox[21], buffer);
    uart_str(buffer);
    uart_endl();
    
    uart_str(" > buffer offset X: \t");
    uint32_to_ascii(mbox[25], buffer);
    uart_str(buffer);
    uart_endl();

    uart_str(" > buffer offset Y: \t");
    uint32_to_ascii(mbox[26], buffer);
    uart_str(buffer);
    uart_endl();
    
    uart_str(" > depth: \t\t");
    uint32_to_ascii(mbox[30], buffer);
    uart_str(buffer);
    uart_endl();
    
    uart_str(" > pixel order: \t");
    if (mbox[34] == 0x0) uart_line("BGR"); else uart_line("RGB");

    uart_str(" > pitch: \t\t");
    uint32_to_ascii(mbox[38], buffer);
    uart_str(buffer);
    uart_endl();
}
