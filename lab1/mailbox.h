#include "gpio.h"
extern volatile unsigned int mailbox[36];

/*channels*/
#define mailbox_power 0
#define mailbox_framebuffer 1
#define mailbox_virtual_uart 2
#define mailbox_vchiq 3
#define mailbox_leds 4
#define mailbox_buttons 5
#define mailbox_touch_screen 6
#define mailbox_count 7
#define mailbox_prop_arm_vc 8

/*mailbox address and flags*/
//#define MMIO_BASE       0x3f000000
#define MAILBOX_BASE    MMIO_BASE + 0xb880

#define MAILBOX_READ    ((volatile unsigned int*) (MAILBOX_BASE))
#define MAILBOX_STATUS  ((volatile unsigned int*) (MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE   ((volatile unsigned int*) (MAILBOX_BASE + 0x20))

#define MAILBOX_EMPTY       0x40000000
#define MAILBOX_FULL        0x80000000
#define MAILBOX_RESPONSE    0x80000000


/*rpi3’s board revision number tags*/
#define GET_BOARD_REVISION  0x00010002
#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

/*arm mem*/
#define GET_ARM_MEM         0x00010005

int mailbox_call();
void get_board_revision();
void get_arm_mem();
