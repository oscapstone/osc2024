#include <bsp//mailbox.h>
#include <bsp/fbdev.h>
#include <bsp/mailbox.h>
#include <bsp/uart.h>
#include <lib/string.h>

unsigned int __attribute__((aligned(16))) mbox[36];

unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
unsigned char *lfb;

// This function is responsible for setting up the framebuffer
int fb_init() {
    mbox[0] = 35 * 4;  // Total size of the message
    mbox[1] = MBOX_REQUEST;

    // Set physical width-height
    mbox[2] = 0x48003;
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = 1024;  // width
    mbox[6] = 768;   // height

    // Set virtual width-height
    mbox[7] = 0x48004;
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = 1024;  // virtual width
    mbox[11] = 768;   // virtual height

    // Set virtual offset
    mbox[12] = 0x48009;
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0;  // x offset
    mbox[16] = 0;  // y offset

    // Set depth
    mbox[17] = 0x48005;
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32;  // depth

    // Set pixel order
    mbox[21] = 0x48006;
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1;  // RGB, not BGR

    // Get framebuffer
    mbox[25] = 0x40001;
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096;  // pointer to framebuffer
    mbox[29] = 0;     // size

    // Get pitch
    mbox[30] = 0x40008;
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0;  // pitch

    mbox[34] = MBOX_TAG_LAST;

    // Call the mailbox
    if (mailbox_call(MBOX_CH_PROP, mbox) && mbox[20] == 32 && mbox[28] != 0) {
        mbox[28] &= 0x3FFFFFFF;  // Convert GPU address to ARM address

        width = mbox[5];   // get actual physical width
        height = mbox[6];  // get actual physical height
        pitch = mbox[33];  // get number of bytes per line
        isrgb = mbox[24];  // get the actual channel order
        lfb = (void *)((unsigned long)mbox[28]);

        // Use the framebuffer (width, height, pitch, isrgb, lfb)
        return 0;  // Success
    }

    return -1;  // Failure
}

int fb_write(size_t pos, const void *buf, size_t len) {
    if (len + pos > pitch * height) {
        uart_puts("[ERROR] Attempted write out of bounds\r\n");
        return -1;
    }

    memcpy(lfb + pos, buf, len);
    return 0;
}
