#include "system.h"
#include "uart.h"
#include "mbox.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC PHYS_TO_VIRT(0x3F10001c)
#define PM_WDOG PHYS_TO_VIRT(0x3F100024)

void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

void reset(int tick) {                 // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    set(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset() {
    set(PM_RSTC, PM_PASSWORD | 0);  // full reset
    set(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}
void reboot()
{
    //disable_uart();
    reset(1); // timeout = 1/16th of a second? (whatever)
} 
int get_board_revision(unsigned int* board_revision)
{
    /*
        GET_BOARD_REVISION
    */
    mbox[0] = 7*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // request code
    mbox[2] = GET_BOARD_REVISION;   // tag identifier
    mbox[3] = 4;                    // value buffer size in bytes
    mbox[4] = 0;                    // request codes : b31 clear, b30-b0 reversed
    mbox[5] = 0;                    // clear output buffer
    mbox[6] = MBOX_TAG_LAST;        // end tag
    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP)) {
        *board_revision = mbox[5];
        return 0;
    } else {
        uart_puts("Unable to query serial!");
        *board_revision = mbox[5] = -1;
        return -1;
    }
}

int get_arm_memory_info(unsigned int* base_addr,unsigned int* size)
{
    /*
        GET arm_memory address and size
    */
    mbox[0] = 8*4;                  // length of the message
    mbox[1] = MBOX_REQUEST;         // request code
    mbox[2] = GET_ARM_MEMORY;       // tag identifier
    mbox[3] = 8;                    // value buffer size in bytes
    mbox[4] = 0;                    // request codes : b31 clear, b30-b0 reversed
    mbox[5] = 0;                    // clear output buffer ( u32: base address in bytes )
    mbox[6] = 0;                    // clear output buffer ( u32: size in bytes )
    mbox[7] = MBOX_TAG_LAST;        // end tag
    
    // send the message to the GPU and receive answer
    if (mbox_call(MBOX_CH_PROP)) {
        *base_addr = mbox[5];
        *size = mbox[6];
        return 0;
    } else {
        uart_puts("Unable to query serial!");
        return -1;
    }
}
