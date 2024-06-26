/***
    Buffer contents:

    u32: size in bytes, (x+1) * 4, including the end tag
    u32: Request code / response code
    u8 ...: sequence of concatenated tags
    u32: the end tag
    u8 ...: padding


    Tag format:

    u32: tag identifier
    u32: value buffer size in bytes
    u32: request code / response code
        b31 = 0: request, b30-b0: reserved
        b31 = 1: response, b30-b0: value length in bytes
    u8 ...: value buffer
    u8 ...: padding to align to 32 bits
***/


#ifndef __MAILBOX_H__
#define __MAILBOX_H__

#define MBOX_RESPONSE               0x80000000

#define MBOX_EMPTY                  0x40000000
#define MBOX_FULL                   0x80000000

#define MBOX_REQUEST                0x00000000
#define MBOX_RESPONSE_SUCCEED       0x80000000
#define MBOX_RESPONSE_FAILED        0x80000001

/* channels */
#define MBOX_CH_POWER  				0
#define MBOX_CH_FB      			1
#define MBOX_CH_VUART   			2
#define MBOX_CH_VCHIQ   			3
#define MBOX_CH_LEDS    			4
#define MBOX_CH_BTNS    			5
#define MBOX_CH_TOUCH   			6
#define MBOX_CH_COUNT   			7
#define MBOX_CH_PROP    			8       // we only use channel 8 (CPU->GPU)

/* tags */

#define MBOX_TAG_GETFWREVISION      0x00000001      // u32: firmware revision 
#define MBOX_TAG_GETMODEL           0x00010001      // u32: board model
#define MBOX_TAG_GETREVISION        0x00010002      // u32: board revision
#define MBOX_TAG_GETMACADDR         0x00010003      // u8 * 6: MAC address in network byte order
#define MBOX_TAG_GETSERIAL          0x00010004      // u64: board serial
#define MBOX_TAG_GETMEMORY          0x00010005      // u32: base, u32: size
#define MBOX_TAG_GETVCMEM           0x00010006      // u32: base, u32: size
#define MBOX_TAG_GETCLOCKS          0x00010007      // u32: parent clock id, u32: clock id, (repeated)

#define MBOX_TAG_ALLOCATEFB         0x00040001      // (u32: alignment in bytes) -> u32: base address, u32: size

#define MBOX_TAG_GETPHYSICALWH	    0x00040003      // u32: width in pixels, u32: height in pixels
#define MBOX_TAG_GETVIRTUALWH    	0x00040004      // u32: width in pixels, u32: height in pixels
#define MBOX_TAG_GETDEPTH           0x00040005      // u32: bits per pixel
#define MBOX_TAG_GETPIXELORDER      0x00040006      // u32: 0x0=BGR, 0x1=RGB
#define MBOX_TAG_GETALPHAMODE       0x00040007      // u32: 0x0=enalbed, 0x1=reversed, 0x2=ignored
#define MBOX_TAG_GETPITCH           0x00040008      // u32: bytes per line
#define MBOX_TAG_GETVIRTUALOFFSET   0x00040009      // u32: X in pixels, u32: Y in pixels
#define MBOX_TAG_GETOVERSCAN        0x0004000a  
#define MBOX_TAG_GETPALETTE         0x0004000b

#define MBOX_TAG_REQUEST            0x00000000
#define MBOX_TAG_END                0x00000000

/* a properly aligned buffer */
extern volatile unsigned int mailbox[36];

uint32_t mailbox_call(unsigned char ch, volatile unsigned int *mbox);

#endif