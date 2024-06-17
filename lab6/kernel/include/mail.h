#define MMIO_BASE       0x3F000000 + 0xffff000000000000
#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#define MAILBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MAILBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x10))
#define MAILBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x14))
#define MAILBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MAILBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x1C))
#define MAILBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))
#define MAILBOX_RESPONSE   0x80000000
#define MAILBOX_FULL       0x80000000
#define MAILBOX_EMPTY      0x40000000

#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_MEMORY     	0x00010005 
#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

unsigned int get_board_revision();
int get_arm_memory(unsigned int*);
int mailbox_call(unsigned int*, unsigned char);
