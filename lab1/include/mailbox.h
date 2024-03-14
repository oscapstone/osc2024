extern volatile unsigned int mbox[36];

#define MBOX_REQUEST    0

/* channels */
#define MBOX_CH_POWER   0 // Mailbox Channel 0: Power Management Interface
#define MBOX_CH_FB      1 // Mailbox Channel 1: Frame Buffer
#define MBOX_CH_VUART   2 // Mailbox Channel 2: Virtual UART
#define MBOX_CH_VCHIQ   3 // Mailbox Channel 3: VCHIQ Interface
#define MBOX_CH_LEDS    4 // Mailbox Channel 4: LEDs Interface
#define MBOX_CH_BTNS    5 // Mailbox Channel 5: Buttons Interface
#define MBOX_CH_TOUCH   6 // Mailbox Channel 6: Touchscreen Interface
#define MBOX_CH_COUNT   7 // Mailbox Channel 7: Counter
#define MBOX_CH_PROP    8 // Mailbox Channel 8: Tags (ARM to VC) , we only use this channel for communication

/* tags */
#define GET_ARM_MEMORY         0x10005
#define MBOX_TAG_GETSERIAL      0x10004
#define GET_BOARD_REVISION      0x10002
#define MBOX_TAG_LAST           0

int mbox_call(unsigned char ch);
void get_board_revision();
void get_ARM_memory();