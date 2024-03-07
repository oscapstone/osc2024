#include "gpio.h"
#include "mini_uart.h"
#include "string.h"


/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[36];

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)

#define MBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))

#define MBOX_RESPONSE   0x80000000

#define MBOX_EMPTY      0x40000000
#define MBOX_FULL       0x80000000

#define MBOX_REQUEST    0

/* channels */
#define MBOX_CH_POWER   0
#define MBOX_CH_FB      1
#define MBOX_CH_VUART   2
#define MBOX_CH_VCHIQ   3
#define MBOX_CH_LEDS    4
#define MBOX_CH_BTNS    5
#define MBOX_CH_TOUCH   6
#define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8       // we only use channel 8 (CPU->GPU)

/* tags */
#define MBOX_TAG_GETSERIAL      0x10004
#define MBOX_TAG_LAST           0


/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mailbox_call(unsigned char ch)
{
    // step 1. Combine the message address (upper 28 bits) with channel number (lower 4 bits) 
    unsigned int r = (((unsigned int)((unsigned long) &mbox) & ~0xF) | (ch & 0xF));

    // step 2. Check if Mailbox 0 status register's full flag is set.
    /* wait until we can write to the mailbox */
    do { asm volatile("nop"); } while (*MBOX_STATUS & MBOX_FULL);
    
    // step 3. If not, then you can write to Mailbox 1 Read/Write register.
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    
    /* now wait for the response */
    while(1) {
        
        // 4. Check if Mailbox 0 status register's empty flag is set.
        /* is there a response? */
        do { asm volatile("nop"); } while (*MBOX_STATUS & MBOX_EMPTY);
        
        // 5. If not, then you can read from Mailbox 0 Read/Write register.
        /* is it a response to our message? */
        if (r == *MBOX_READ) {
            // 6. Check if the value is the same as you wrote in step 1.
            /* is it a valid successful response? */
            return mbox[1] == MBOX_RESPONSE;
        }
    }
    return 0;
}


/* mailbox tags */
#define GET_FIRMWARE_REVISION       0x00000001      // u32: firmware revision 
#define GET_BOARD_MODEL             0x00010001      // u32: board model
#define GET_BOARD_REVISION          0x00010002      // u32: board revision
#define GET_BOARD_MAC_ADDRESS       0x00010003      // u8 * 6: MAC address in network byte order
#define GET_BOARD_SERIAL            0x00010004      // u64: board serial
#define GET_ARM_MEMORY              0x00010005      // u32: base, u32: size
#define GET_VC_MEMORY               0x00010006      // u32: base, u32: size
#define GET_CLOCKS                  0x00010007      // u32: parent clock id, u32: clock id, (repeated)

#define GET_PHYSICAL_WIDTH_HEIGHT   0x00040003      // u32: width in pixels, u32: height in pixels
#define GET_VIRTUAL_WIDTH_HEIGHT    0x00040004      // u32: width in pixels, u32: height in pixels
#define GET_DEPTH                   0x00040005      // u32: bits per pixel
#define GET_PIXEL_ORDER             0x00040006      // u32: 0x0=BGR, 0x1=RGB
#define GET_ALPHA_MODE              0x00040007      // u32: 0x0=enalbed, 0x1=reversed, 0x2=ignored
#define GET_PITCH                   0x00040008      // u32: bytes per line
#define GET_VIRTUAL_OFFSET          0x00040009      // u32: X in pixels, u32: Y in pixels
#define GET_OVERSCAN                0x0004000a  
#define GET_PALETTE                 0x0004000b

#define REQUEST_CODE                0x00000000
#define REQUEST_SUCCEED             0x80000000
#define REQUEST_FAILED              0x80000001

#define TAG_REQUEST_CODE            0x00000000
#define END_TAG                     0x00000000


void mailbox_get_board_revision()
{
    mbox[0] = 7 * 4;                                // buffer size in bytes
    mbox[1] = REQUEST_CODE;
    
    // tags begin
    mbox[2] = GET_BOARD_REVISION;                   // tag identifier
    mbox[3] = 4;                                    // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;                                    // value buffer
    mbox[6] = END_TAG;                              // tags end
    
    mailbox_call(MBOX_CH_PROP);    // message passing procedure call, you should implement it following the 6 steps provided above.
 
    mini_uart_puts("Board revision: \t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[5]);                         // it should be 0xa020d3 for rpi3 b+
    mini_uart_puts("\r\n");
}

void mailbox_get_arm_memory()
{
    mbox[0] = 8 * 4;                                // buffer size in bytes
    mbox[1] = REQUEST_CODE;

    // tags begin
    mbox[2] = GET_ARM_MEMORY;                       // tag identifier
    mbox[3] = 8;                                    // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;                                    // value buffer
    mbox[6] = 0;                                    // value buffer
    mbox[7] = END_TAG;                              // tags end
    
    mailbox_call(MBOX_CH_PROP); // message passing procedure call, you should implement it following the 6 steps provided above.
 
    mini_uart_putln("Arm memory memory info:");

    mini_uart_puts("-> base: \t\t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[5]);
    mini_uart_puts("\r\n");

    mini_uart_puts("-> size: \t\t");
    mini_uart_puts("0x");
    mini_uart_hex(mbox[6]);
    mini_uart_puts("\r\n");
}


void mailbox_get_vc_info()
{
    char buffer[32];

    /* 1 */
    mbox[0] = 8 * 4;                                // buffer size in bytes
    mbox[1] = REQUEST_CODE;

    // tags begin
    mbox[2] = GET_VC_MEMORY;                        // tag identifier
    mbox[3] = 8;                                    // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;                                    // value buffer
    mbox[6] = 0;                                    // value buffer
    mbox[7] = END_TAG;                              // tags end
    
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


    /* 2 */
    mbox[0] = 8 * 4;                                // buffer size in bytes
    mbox[1] = REQUEST_CODE;

    // tags begin
    mbox[2] = GET_PHYSICAL_WIDTH_HEIGHT;            // tag identifier
    mbox[3] = 8;                                    // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;                                    // value buffer
    mbox[6] = 0;                                    // value buffer
    mbox[7] = END_TAG;                              // tags end
    
    mailbox_call(MBOX_CH_PROP); // message passing procedure call, you should implement it following the 6 steps provided above.

    mini_uart_puts("-> display width: \t");
    uint_to_ascii(mbox[5], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n");

    mini_uart_puts("-> display height: \t");
    uint_to_ascii(mbox[6], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n"); 


    /* 3 */
    mbox[0] = 8 * 4;                                // buffer size in bytes
    mbox[1] = REQUEST_CODE;

    // tags begin
    mbox[2] = GET_VIRTUAL_WIDTH_HEIGHT;             // tag identifier
    mbox[3] = 8;                                    // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;                                    // value buffer
    mbox[6] = 0;                                    // value buffer
    mbox[7] = END_TAG;                              // tags end
    
    mailbox_call(MBOX_CH_PROP); // message passing procedure call, you should implement it following the 6 steps provided above.
    
    mini_uart_puts("-> buffer width: \t");
    uint_to_ascii(mbox[5], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n");

    mini_uart_puts("-> buffer height: \t");
    uint_to_ascii(mbox[6], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n"); 


    /* 4 */
    mbox[0] = 8 * 4;                                // buffer size in bytes
    mbox[1] = REQUEST_CODE;

    // tags begin
    mbox[2] = GET_VIRTUAL_OFFSET;                   // tag identifier
    mbox[3] = 8;                                    // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;                                    // value buffer
    mbox[6] = 0;                                    // value buffer
    mbox[7] = END_TAG;                              // tags end
    
    mailbox_call(MBOX_CH_PROP); // message passing procedure call, you should implement it following the 6 steps provided above.
    
    mini_uart_puts("-> buffer offset X: \t");
    uint_to_ascii(mbox[5], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n");

    mini_uart_puts("-> buffer offset Y: \t");
    uint_to_ascii(mbox[6], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n"); 


    /* 5 */
    mbox[0] = 7 * 4;                                // buffer size in bytes
    mbox[1] = REQUEST_CODE;

    // tags begin
    mbox[2] = GET_DEPTH;                            // tag identifier
    mbox[3] = 4;                                    // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;                                    // value buffer
    mbox[6] = END_TAG;                              // tags end
    
    mailbox_call(MBOX_CH_PROP); // message passing procedure call, you should implement it following the 6 steps provided above.
    
    mini_uart_puts("-> depth: \t\t");
    uint_to_ascii(mbox[5], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n");


    /* 6 */
    mbox[0] = 7 * 4;                                // buffer size in bytes
    mbox[1] = REQUEST_CODE;

    // tags begin
    mbox[2] = GET_PIXEL_ORDER;                            // tag identifier
    mbox[3] = 4;                                    // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;                                    // value buffer
    mbox[6] = END_TAG;                              // tags end
    
    mailbox_call(MBOX_CH_PROP); // message passing procedure call, you should implement it following the 6 steps provided above.
    
    mini_uart_puts("-> pixel order: \t");
    if (mbox[5] == 0x0) mini_uart_putln("BGR"); else mini_uart_putln("RGB");


    /* 7 */
    mbox[0] = 7 * 4;                                // buffer size in bytes
    mbox[1] = REQUEST_CODE;

    // tags begin
    mbox[2] = GET_PITCH;                            // tag identifier
    mbox[3] = 4;                                    // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;                                    // value buffer
    mbox[6] = END_TAG;                              // tags end
    
    mailbox_call(MBOX_CH_PROP); // message passing procedure call, you should implement it following the 6 steps provided above.
    
    mini_uart_puts("-> pitch: \t\t");
    uint_to_ascii(mbox[5], buffer);
    mini_uart_puts(buffer);
    mini_uart_puts("\r\n");
}


