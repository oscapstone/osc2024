/* a properly aligned buffer */
extern volatile unsigned int mbox[8];

#define MBOX_REQUEST    0
#define TAG_REQUEST_CODE    0

/* channels */
#define MBOX_CH_PROP    8

/* tags */
#define MBOX_TAG_GETREVISION    0x10002
#define MBOX_TAG_GETARMMEM      0x10005
#define MBOX_TAG_LAST           0

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#define MBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000


int mbox_call(unsigned char ch);
void show_info();